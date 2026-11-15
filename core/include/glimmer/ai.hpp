#pragma once

/// @file ai.hpp
/// Everything AI-related that doesn't need a network: building prompts from
/// entries, and encoding/decoding the OpenAI-compatible chat-completions
/// format that OmniRoute (or any compatible gateway) speaks.
///
/// The HTTP call itself lives in the app layer (Qt networking), which keeps
/// this file fully unit-testable.

#include "glimmer/date.hpp"
#include "glimmer/entry.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace glimmer::ai {

/// Thrown when the gateway returns an error or an unexpected payload.
class AiError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

struct ChatMessage {
  std::string role;  ///< "system", "user" or "assistant".
  std::string content;
  friend bool operator==(const ChatMessage&, const ChatMessage&) = default;
};

struct ChatRequest {
  std::string model = "auto";
  std::vector<ChatMessage> messages;
  double temperature = 0.7;
  std::optional<int> maxTokens;
};

struct ChatResult {
  std::string content;
  std::string model;  ///< The model that actually answered, if reported.
};

/// Serializes a request body for POST {baseUrl}/chat/completions.
[[nodiscard]] std::string encodeChatRequest(const ChatRequest& request);

/// Extracts choices[0].message.content from a response body.
/// Throws AiError with the gateway's message on `{"error": {...}}` bodies.
[[nodiscard]] ChatResult decodeChatResponse(std::string_view body);

/// Renders entries as plain text for a prompt, newest content kept when the
/// result would exceed `maxChars` (older entries are dropped first).
[[nodiscard]] std::string renderEntries(const std::vector<Entry>& entries,
                                        std::size_t maxChars = 24'000);

/// Prompt templates. All of them start with the same system prompt, which
/// keeps the assistant supportive, brief, and honest about what it can see.
class PromptBuilder {
 public:
  explicit PromptBuilder(std::string model = "auto") : model_(std::move(model)) {}

  /// "Summarise what I wrote between `from` and `to`."
  [[nodiscard]] ChatRequest summarise(const std::vector<Entry>& entries, Date from, Date to) const;

  /// "Give me something to write about today", informed by recent entries.
  [[nodiscard]] ChatRequest journalingPrompt(const std::vector<Entry>& recentEntries,
                                             Date today) const;

  /// A free-form question about the given entries.
  [[nodiscard]] ChatRequest ask(std::string_view question, const std::vector<Entry>& entries) const;

  [[nodiscard]] static std::string_view systemPrompt() noexcept;

 private:
  std::string model_;
};

}  // namespace glimmer::ai
