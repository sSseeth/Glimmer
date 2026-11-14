#include "glimmer/ai.hpp"

#include "glimmer/json.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using glimmer::Date;
using glimmer::Entry;
using glimmer::EntryKind;
namespace ai = glimmer::ai;
namespace json = glimmer::json;

namespace {

Entry entry(Date date, std::string body, EntryKind kind = EntryKind::Text) {
  Entry e;
  e.date = date;
  e.kind = kind;
  e.body = std::move(body);
  return e;
}

constexpr Date kNov09{year{2026}, November, day{9}};
constexpr Date kNov14{year{2026}, November, day{14}};
constexpr Date kNov15{year{2026}, November, day{15}};

}  // namespace

TEST(AiCodec, EncodesOpenAiCompatibleRequest) {
  ai::ChatRequest request;
  request.model = "auto";
  request.temperature = 0.5;
  request.maxTokens = 200;
  request.messages = {{"system", "be kind"}, {"user", "hi \"there\""}};

  const auto body = json::parse(ai::encodeChatRequest(request));
  EXPECT_EQ(body.find("model")->asString(), "auto");
  EXPECT_DOUBLE_EQ(body.find("temperature")->asNumber(), 0.5);
  EXPECT_DOUBLE_EQ(body.find("max_tokens")->asNumber(), 200.0);
  EXPECT_FALSE(body.find("stream")->asBool());
  const auto& messages = body.find("messages")->asArray();
  ASSERT_EQ(messages.size(), 2U);
  EXPECT_EQ(messages[1].find("role")->asString(), "user");
  EXPECT_EQ(messages[1].find("content")->asString(), "hi \"there\"");
}

TEST(AiCodec, OmitsMaxTokensWhenUnset) {
  ai::ChatRequest request;
  const auto body = json::parse(ai::encodeChatRequest(request));
  EXPECT_EQ(body.find("max_tokens"), nullptr);
}

TEST(AiCodec, DecodesSuccessfulResponse) {
  const auto result = ai::decodeChatResponse(R"({
    "id": "chatcmpl-1", "object": "chat.completion", "model": "some-provider/model-x",
    "choices": [{"index": 0, "message": {"role": "assistant", "content": "A good week."},
                 "finish_reason": "stop"}]
  })");
  EXPECT_EQ(result.content, "A good week.");
  EXPECT_EQ(result.model, "some-provider/model-x");
}

TEST(AiCodec, SurfacesGatewayErrors) {
  try {
    (void)ai::decodeChatResponse(R"({"error": {"message": "invalid api key", "code": 401}})");
    FAIL() << "expected AiError";
  } catch (const ai::AiError& e) {
    EXPECT_NE(std::string{e.what()}.find("invalid api key"), std::string::npos);
  }
}

TEST(AiCodec, RejectsMalformedResponses) {
  EXPECT_THROW((void)ai::decodeChatResponse("not json"), ai::AiError);
  EXPECT_THROW((void)ai::decodeChatResponse(R"({"choices": []})"), ai::AiError);
  EXPECT_THROW((void)ai::decodeChatResponse(R"({"choices": [{"message": {}}]})"), ai::AiError);
}

TEST(RenderEntries, FormatsChronologicallyWithDatesAndKinds) {
  auto collage = entry(kNov15, "beach trip", EntryKind::Collage);
  collage.collage.resize(3);
  const auto text = ai::renderEntries({entry(kNov14, "Went running"), collage});
  const auto first = text.find("## 2026-11-14 (text)");
  const auto second = text.find("## 2026-11-15 (collage)");
  ASSERT_NE(first, std::string::npos);
  ASSERT_NE(second, std::string::npos);
  EXPECT_LT(first, second);
  EXPECT_NE(text.find("[collage of 3 image(s)] beach trip"), std::string::npos);
}

TEST(RenderEntries, DropsOldestEntriesWhenOverBudget) {
  const std::vector<Entry> entries{entry(kNov09, std::string(500, 'o')), entry(kNov14, "newest")};
  const auto text = ai::renderEntries(entries, 100);
  EXPECT_NE(text.find("newest"), std::string::npos);
  EXPECT_EQ(text.find("ooooo"), std::string::npos);
  EXPECT_NE(text.find("1 older entries omitted"), std::string::npos);
}

TEST(PromptBuilder, SummariseIncludesRangeEntriesAndSystemPrompt) {
  const ai::PromptBuilder builder{"my-model"};
  const auto request = builder.summarise({entry(kNov14, "Saw a firefly")}, kNov09, kNov15);
  EXPECT_EQ(request.model, "my-model");
  ASSERT_EQ(request.messages.size(), 2U);
  EXPECT_EQ(request.messages[0].role, "system");
  EXPECT_EQ(request.messages[0].content, ai::PromptBuilder::systemPrompt());
  EXPECT_NE(request.messages[1].content.find("2026-11-09 to 2026-11-15"), std::string::npos);
  EXPECT_NE(request.messages[1].content.find("Saw a firefly"), std::string::npos);
}

TEST(PromptBuilder, SummariseHandlesEmptyPeriods) {
  const auto request = ai::PromptBuilder{}.summarise({}, kNov09, kNov15);
  EXPECT_NE(request.messages[1].content.find("no entries"), std::string::npos);
}

TEST(PromptBuilder, JournalingPromptMentionsToday) {
  const auto request = ai::PromptBuilder{}.journalingPrompt({}, kNov14);
  EXPECT_NE(request.messages[1].content.find("2026-11-14"), std::string::npos);
}

TEST(PromptBuilder, AskEmbedsQuestion) {
  const auto request = ai::PromptBuilder{}.ask("When did I last run?", {entry(kNov14, "ran 5k")});
  EXPECT_NE(request.messages[1].content.find("When did I last run?"), std::string::npos);
  EXPECT_NE(request.messages[1].content.find("ran 5k"), std::string::npos);
}
