#include "glimmer/json.hpp"

#include <gtest/gtest.h>

namespace json = glimmer::json;

TEST(Json, ParsesScalars) {
  EXPECT_TRUE(json::parse("null").isNull());
  EXPECT_TRUE(json::parse("true").asBool());
  EXPECT_FALSE(json::parse("false").asBool());
  EXPECT_DOUBLE_EQ(json::parse("-12.5e1").asNumber(), -125.0);
  EXPECT_DOUBLE_EQ(json::parse("0").asNumber(), 0.0);
  EXPECT_EQ(json::parse(R"("hi")").asString(), "hi");
}

TEST(Json, ParsesNestedStructures) {
  const auto v = json::parse(R"( { "a": [1, {"b": "c"}], "d": {} } )");
  ASSERT_TRUE(v.isObject());
  const auto* a = v.find("a");
  ASSERT_NE(a, nullptr);
  ASSERT_EQ(a->asArray().size(), 2U);
  EXPECT_EQ(a->at(1)->find("b")->asString(), "c");
  EXPECT_TRUE(v.find("d")->asObject().empty());
  EXPECT_EQ(v.find("missing"), nullptr);
  EXPECT_EQ(a->at(5), nullptr);
}

TEST(Json, DecodesEscapesAndUnicode) {
  EXPECT_EQ(json::parse(R"("a\"b\\c\/d\n\t")").asString(), "a\"b\\c/d\n\t");
  EXPECT_EQ(json::parse(R"("\u00e9")").asString(), "\xC3\xA9");                // é
  EXPECT_EQ(json::parse(R"("\u65e5")").asString(), "\xE6\x97\xA5");            // 日
  EXPECT_EQ(json::parse(R"("\ud83d\ude00")").asString(), "\xF0\x9F\x98\x80");  // 😀
  EXPECT_EQ(json::parse("\"\xE2\x9C\xA8\"").asString(), "\xE2\x9C\xA8");       // raw UTF-8 passes
}

TEST(Json, RejectsInvalidDocuments) {
  for (const char* bad : {"", "{", "[1,]", "{\"a\":}", "01", "1.", "-", "tru", "\"\\x\"",
                          "\"\\ud800\"", "\"line\nbreak\"", "[1] 2", "{'a': 1}", "NaN"}) {
    EXPECT_THROW((void)json::parse(bad), json::ParseError) << "input: " << bad;
  }
}

TEST(Json, RejectsExcessiveNesting) {
  const std::string deep(1000, '[');
  EXPECT_THROW((void)json::parse(deep), json::ParseError);
}

TEST(Json, SerializesCompactlyInInsertionOrder) {
  const json::Value v{json::Value::Object{
      {"z", 1},
      {"a", json::Value::Array{true, nullptr, 2.5, "x"}},
  }};
  EXPECT_EQ(json::serialize(v), R"({"z":1,"a":[true,null,2.5,"x"]})");
}

TEST(Json, SerializeEscapesControlCharacters) {
  const json::Value v{std::string{"quote\" slash\\ nl\n bell\a"}};
  EXPECT_EQ(json::serialize(v), R"("quote\" slash\\ nl\n bell\u0007")");
}

TEST(Json, RoundTripsThroughSerialize) {
  const auto text = R"({"model":"auto","messages":[{"role":"user","content":"héllo\n"}],"n":3})";
  EXPECT_EQ(json::serialize(json::parse(text)), text);
  EXPECT_EQ(json::parse(json::serialize(json::parse(text))), json::parse(text));
}
