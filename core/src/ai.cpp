#include "glimmer/ai.hpp"

#include "glimmer/json.hpp"

#include <deque>
#include <ranges>

namespace glimmer::ai {

std::string encodeChatRequest(const ChatRequest& request) {
  json::Value::Array messages;
  messages.reserve(request.messages.size());
  for (const auto& message : request.messages) {
    messages.emplace_back(json::Value::Object{
        {"role", message.role},
        {"content", message.content},
    });
  }

  json::Value::Object body{
      {"model", request.model},
      {"messages", std::move(messages)},
      {"temperature", request.temperature},
      {"stream", false},
  };
  if (request.maxTokens) {
    body.emplace_back("max_tokens", *request.maxTokens);
  }
  return json::serialize(json::Value{std::move(body)});
}

ChatResult decodeChatResponse(std::string_view body) {
  json::Value root;
  try {
    root = json::parse(body);
  } catch (const json::ParseError& e) {
    throw AiError(std::string{"gateway returned invalid JSON: "} + e.what());
  }

  if (const auto* error = root.find("error"); error != nullptr && !error->isNull()) {
    const auto* message = error->find("message");
    if (message != nullptr && message->isString()) {
      throw AiError("gateway error: " + message->asString());
    }
    if (error->isString()) {
      throw AiError("gateway error: " + error->asString());
    }
    throw AiError("gateway returned an error");
  }

  const auto* choices = root.find("choices");
  const auto* first = (choices != nullptr) ? choices->at(0) : nullptr;
  const auto* message = (first != nullptr) ? first->find("message") : nullptr;
  const auto* content = (message != nullptr) ? message->find("content") : nullptr;
  if (content == nullptr || !content->isString()) {
    throw AiError("gateway response had no choices[0].message.content");
  }

  ChatResult result;
  result.content = content->asString();
  if (const auto* model = root.find("model"); model != nullptr && model->isString()) {
    result.model = model->asString();
  }
  return result;
}

std::string renderEntries(const std::vector<Entry>& entries, std::size_t maxChars) {
  // Walk newest-to-oldest, keep what fits, then emit in chronological order.
  std::deque<std::string> blocks;
  std::size_t total = 0;
  for (const auto& entry : std::views::reverse(entries)) {
    std::string block =
        "## " + toIsoString(entry.date) + " (" + std::string{toString(entry.kind)} + ")";
    if (!entry.title.empty()) {
      block += " - " + entry.title;
    }
    block += '\n';
    if (entry.kind == EntryKind::Collage) {
      block += "[collage of " + std::to_string(entry.collage.size()) + " image(s)]";
      if (!entry.body.empty()) {
        block += ' ' + entry.body;
      }
    } else if (entry.body.empty()) {
      block += "[no text]";
    } else {
      block += entry.body;
    }
    block += "\n\n";

    if (total + block.size() > maxChars) {
      break;
    }
    total += block.size();
    blocks.push_front(std::move(block));
  }

  std::string out;
  out.reserve(total);
  for (const auto& block : blocks) {
    out += block;
  }
  if (blocks.size() < entries.size()) {
    out = "[" + std::to_string(entries.size() - blocks.size()) +
          " older entries omitted for length]\n\n" + out;
  }
  return out;
}

std::string_view PromptBuilder::systemPrompt() noexcept {
  return "You are a gentle, concise journaling companion inside a private diary app. "
         "You only know what is in the entries you are shown. "
         "Refer to entries by their date. Do not invent events. "
         "Do not diagnose or give medical, legal or financial advice; if the writer seems "
         "to be struggling, encourage them kindly to reach out to someone they trust. "
         "Write in plain prose, in the same language as the entries.";
}

ChatRequest PromptBuilder::summarise(const std::vector<Entry>& entries, Date from, Date to) const {
  ChatRequest request;
  request.model = model_;
  request.temperature = 0.3;
  request.messages.push_back({"system", std::string{systemPrompt()}});

  std::string user = "Summarise my diary from " + toIsoString(from) + " to " + toIsoString(to) +
                     ". Give a short overview paragraph, then up to five bullet points of "
                     "notable moments, then one line about recurring themes or moods.\n\n";
  if (entries.empty()) {
    user += "(There are no entries in this period.)";
  } else {
    user += renderEntries(entries);
  }
  request.messages.push_back({"user", std::move(user)});
  return request;
}

ChatRequest PromptBuilder::journalingPrompt(const std::vector<Entry>& recentEntries,
                                            Date today) const {
  ChatRequest request;
  request.model = model_;
  request.temperature = 0.9;
  request.messages.push_back({"system", std::string{systemPrompt()}});

  std::string user = "Today is " + toIsoString(today) +
                     ". Suggest one short, specific journaling prompt for today (one or two "
                     "sentences, no preamble).";
  if (!recentEntries.empty()) {
    user += " You may build on threads from my recent entries:\n\n" +
            renderEntries(recentEntries, 8'000);
  }
  request.messages.push_back({"user", std::move(user)});
  return request;
}

ChatRequest PromptBuilder::ask(std::string_view question, const std::vector<Entry>& entries) const {
  ChatRequest request;
  request.model = model_;
  request.temperature = 0.5;
  request.messages.push_back({"system", std::string{systemPrompt()}});

  std::string user = "Here are my diary entries:\n\n";
  user += entries.empty() ? std::string{"(none)\n\n"} : renderEntries(entries);
  user += "My question: ";
  user += question;
  request.messages.push_back({"user", std::move(user)});
  return request;
}

}  // namespace glimmer::ai
