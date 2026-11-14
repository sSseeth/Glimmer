#pragma once

/// @file json.hpp
/// A deliberately small JSON value type, parser and serializer.
///
/// We only need JSON to talk to an OpenAI-compatible HTTP API, so this covers
/// RFC 8259 and nothing more (no comments, no trailing commas). See
/// docs/adr/0004-ai-gateway.md for why we didn't pull in a library.

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace glimmer::json {

class ParseError : public std::runtime_error {
 public:
  ParseError(const std::string& message, std::size_t offset);
  [[nodiscard]] std::size_t offset() const noexcept { return offset_; }

 private:
  std::size_t offset_;
};

class Value {
 public:
  using Array = std::vector<Value>;
  /// Objects keep insertion order, which makes serialized output predictable.
  /// (A std::pair rather than a named struct: std::variant needs every
  /// alternative to be usable while Value itself is still being defined.)
  using Member = std::pair<std::string, Value>;
  using Object = std::vector<Member>;

  // Constructors are implicit on purpose, so `Value v = "text";` and
  // `Object{{"key", 1}}` read naturally.
  Value() = default;  // null
  Value(std::nullptr_t) {}
  Value(bool b) : data_(b) {}
  template <typename T>
    requires(std::is_arithmetic_v<T> && !std::same_as<T, bool>)
  Value(T number) : data_(static_cast<double>(number)) {}
  Value(std::string s) : data_(std::move(s)) {}
  Value(std::string_view s) : data_(std::string{s}) {}
  Value(const char* s) : data_(std::string{s}) {}
  Value(Array a) : data_(std::move(a)) {}
  Value(Object o) : data_(std::move(o)) {}

  [[nodiscard]] bool isNull() const noexcept {
    return std::holds_alternative<std::nullptr_t>(data_);
  }
  [[nodiscard]] bool isBool() const noexcept { return std::holds_alternative<bool>(data_); }
  [[nodiscard]] bool isNumber() const noexcept { return std::holds_alternative<double>(data_); }
  [[nodiscard]] bool isString() const noexcept {
    return std::holds_alternative<std::string>(data_);
  }
  [[nodiscard]] bool isArray() const noexcept { return std::holds_alternative<Array>(data_); }
  [[nodiscard]] bool isObject() const noexcept { return std::holds_alternative<Object>(data_); }

  // Throw std::bad_variant_access if the type is wrong.
  [[nodiscard]] bool asBool() const { return std::get<bool>(data_); }
  [[nodiscard]] double asNumber() const { return std::get<double>(data_); }
  [[nodiscard]] const std::string& asString() const { return std::get<std::string>(data_); }
  [[nodiscard]] const Array& asArray() const { return std::get<Array>(data_); }
  [[nodiscard]] const Object& asObject() const { return std::get<Object>(data_); }

  /// Returns the member named `key`, or nullptr if this isn't an object or
  /// has no such member.
  [[nodiscard]] const Value* find(std::string_view key) const;

  /// Returns array element `index`, or nullptr if out of range / not an array.
  [[nodiscard]] const Value* at(std::size_t index) const;

  friend bool operator==(const Value& a, const Value& b);

 private:
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> data_{nullptr};
};

/// Parses a complete JSON document. Throws ParseError.
[[nodiscard]] Value parse(std::string_view text);

/// Serializes to compact JSON (no whitespace).
[[nodiscard]] std::string serialize(const Value& value);

}  // namespace glimmer::json
