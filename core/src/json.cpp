#include "glimmer/json.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>

namespace glimmer::json {

ParseError::ParseError(const std::string& message, std::size_t offset)
    : std::runtime_error(message + " at offset " + std::to_string(offset)), offset_(offset) {}

const Value* Value::find(std::string_view key) const {
  if (!isObject()) {
    return nullptr;
  }
  for (const auto& member : asObject()) {
    if (member.first == key) {
      return &member.second;
    }
  }
  return nullptr;
}

const Value* Value::at(std::size_t index) const {
  if (!isArray() || index >= asArray().size()) {
    return nullptr;
  }
  return &asArray()[index];
}

bool operator==(const Value& a, const Value& b) { return a.data_ == b.data_; }

// --- Parser ------------------------------------------------------------------

namespace {

constexpr int kMaxDepth = 128;

class Parser {
 public:
  explicit Parser(std::string_view text) : text_(text) {}

  Value parseDocument() {
    skipWhitespace();
    Value value = parseValue(0);
    skipWhitespace();
    if (pos_ != text_.size()) {
      fail("unexpected trailing characters");
    }
    return value;
  }

 private:
  [[noreturn]] void fail(const std::string& message) const { throw ParseError(message, pos_); }

  [[nodiscard]] static bool isDigit(char c) noexcept { return c >= '0' && c <= '9'; }

  [[nodiscard]] bool atEnd() const noexcept { return pos_ >= text_.size(); }
  [[nodiscard]] char peek() const { return atEnd() ? '\0' : text_[pos_]; }

  void expect(char c) {
    if (peek() != c) {
      fail(std::string{"expected '"} + c + "'");
    }
    ++pos_;
  }

  void expectLiteral(std::string_view literal) {
    if (text_.substr(pos_, literal.size()) != literal) {
      fail("invalid literal");
    }
    pos_ += literal.size();
  }

  void skipWhitespace() {
    while (!atEnd()) {
      const char c = text_[pos_];
      if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
        break;
      }
      ++pos_;
    }
  }

  Value parseValue(int depth) {
    if (depth > kMaxDepth) {
      fail("nesting too deep");
    }
    switch (peek()) {
      case '{':
        return parseObject(depth);
      case '[':
        return parseArray(depth);
      case '"':
        return Value{parseString()};
      case 't':
        expectLiteral("true");
        return Value{true};
      case 'f':
        expectLiteral("false");
        return Value{false};
      case 'n':
        expectLiteral("null");
        return Value{nullptr};
      default:
        return parseNumber();
    }
  }

  Value parseObject(int depth) {
    expect('{');
    Value::Object object;
    skipWhitespace();
    if (peek() == '}') {
      ++pos_;
      return Value{std::move(object)};
    }
    while (true) {
      skipWhitespace();
      std::string key = parseString();
      skipWhitespace();
      expect(':');
      skipWhitespace();
      object.emplace_back(std::move(key), parseValue(depth + 1));
      skipWhitespace();
      if (peek() == ',') {
        ++pos_;
        continue;
      }
      expect('}');
      return Value{std::move(object)};
    }
  }

  Value parseArray(int depth) {
    expect('[');
    Value::Array array;
    skipWhitespace();
    if (peek() == ']') {
      ++pos_;
      return Value{std::move(array)};
    }
    while (true) {
      skipWhitespace();
      array.push_back(parseValue(depth + 1));
      skipWhitespace();
      if (peek() == ',') {
        ++pos_;
        continue;
      }
      expect(']');
      return Value{std::move(array)};
    }
  }

  Value parseNumber() {
    // Validate the RFC 8259 grammar first, then let from_chars convert.
    const std::size_t start = pos_;
    if (peek() == '-') {
      ++pos_;
    }
    if (peek() == '0') {
      ++pos_;
    } else if (isDigit(peek())) {
      while (isDigit(peek())) {
        ++pos_;
      }
    } else {
      fail("invalid value");
    }
    if (peek() == '.') {
      ++pos_;
      if (!isDigit(peek())) {
        fail("digit expected after decimal point");
      }
      while (isDigit(peek())) {
        ++pos_;
      }
    }
    if (peek() == 'e' || peek() == 'E') {
      ++pos_;
      if (peek() == '+' || peek() == '-') {
        ++pos_;
      }
      if (!isDigit(peek())) {
        fail("digit expected in exponent");
      }
      while (isDigit(peek())) {
        ++pos_;
      }
    }
    double number = 0.0;
    const char* first = text_.data() + start;
    const char* last = text_.data() + pos_;
    const auto [ptr, ec] = std::from_chars(first, last, number);
    if (ec != std::errc{} || ptr != last) {
      fail("number out of range");
    }
    return Value{number};
  }

  unsigned parseHex4() {
    if (pos_ + 4 > text_.size()) {
      fail("truncated \\u escape");
    }
    unsigned code = 0;
    for (int i = 0; i < 4; ++i) {
      const char c = text_[pos_++];
      code <<= 4U;
      if (c >= '0' && c <= '9') {
        code |= static_cast<unsigned>(c - '0');
      } else if (c >= 'a' && c <= 'f') {
        code |= static_cast<unsigned>(c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        code |= static_cast<unsigned>(c - 'A' + 10);
      } else {
        fail("invalid hex digit in \\u escape");
      }
    }
    return code;
  }

  static void appendUtf8(std::string& out, unsigned codepoint) {
    if (codepoint < 0x80U) {
      out += static_cast<char>(codepoint);
    } else if (codepoint < 0x800U) {
      out += static_cast<char>(0xC0U | (codepoint >> 6U));
      out += static_cast<char>(0x80U | (codepoint & 0x3FU));
    } else if (codepoint < 0x10000U) {
      out += static_cast<char>(0xE0U | (codepoint >> 12U));
      out += static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
      out += static_cast<char>(0x80U | (codepoint & 0x3FU));
    } else {
      out += static_cast<char>(0xF0U | (codepoint >> 18U));
      out += static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU));
      out += static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
      out += static_cast<char>(0x80U | (codepoint & 0x3FU));
    }
  }

  std::string parseString() {
    expect('"');
    std::string out;
    while (true) {
      if (atEnd()) {
        fail("unterminated string");
      }
      const char c = text_[pos_++];
      if (c == '"') {
        return out;
      }
      if (static_cast<unsigned char>(c) < 0x20U) {
        fail("control character in string");
      }
      if (c != '\\') {
        out += c;  // UTF-8 bytes pass through untouched.
        continue;
      }
      if (atEnd()) {
        fail("unterminated escape");
      }
      const char escape = text_[pos_++];
      switch (escape) {
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case '/':
          out += '/';
          break;
        case 'b':
          out += '\b';
          break;
        case 'f':
          out += '\f';
          break;
        case 'n':
          out += '\n';
          break;
        case 'r':
          out += '\r';
          break;
        case 't':
          out += '\t';
          break;
        case 'u': {
          unsigned code = parseHex4();
          if (code >= 0xD800U && code <= 0xDBFFU) {
            // High surrogate: must be followed by \uDC00-\uDFFF.
            if (text_.substr(pos_, 2) != "\\u") {
              fail("unpaired surrogate");
            }
            pos_ += 2;
            const unsigned low = parseHex4();
            if (low < 0xDC00U || low > 0xDFFFU) {
              fail("invalid low surrogate");
            }
            code = 0x10000U + ((code - 0xD800U) << 10U) + (low - 0xDC00U);
          } else if (code >= 0xDC00U && code <= 0xDFFFU) {
            fail("unpaired surrogate");
          }
          appendUtf8(out, code);
          break;
        }
        default:
          fail("invalid escape");
      }
    }
  }

  std::string_view text_;
  std::size_t pos_ = 0;
};

// --- Serializer --------------------------------------------------------------

void writeString(std::string& out, const std::string& s) {
  constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5', '6', '7',
                                      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  out += '"';
  for (const char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      default: {
        const auto byte = static_cast<unsigned char>(c);
        if (byte < 0x20U) {
          out += "\\u00";
          out += kHex.at(byte >> 4U);
          out += kHex.at(byte & 0x0FU);
        } else {
          out += c;
        }
      }
    }
  }
  out += '"';
}

void writeNumber(std::string& out, double number) {
  if (!std::isfinite(number)) {
    out += "null";  // JSON has no NaN/Infinity.
    return;
  }
  // to_chars without a format gives the shortest text that round-trips
  // exactly, so whole numbers print as "3" rather than "3.0". No
  // floating-point comparison needed.
  std::array<char, 32> buffer{};
  const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
  out.append(buffer.data(), result.ptr);
}

void write(std::string& out, const Value& value) {
  if (value.isNull()) {
    out += "null";
  } else if (value.isBool()) {
    out += value.asBool() ? "true" : "false";
  } else if (value.isNumber()) {
    writeNumber(out, value.asNumber());
  } else if (value.isString()) {
    writeString(out, value.asString());
  } else if (value.isArray()) {
    out += '[';
    bool first = true;
    for (const auto& element : value.asArray()) {
      if (!first) {
        out += ',';
      }
      first = false;
      write(out, element);
    }
    out += ']';
  } else {
    out += '{';
    bool first = true;
    for (const auto& member : value.asObject()) {
      if (!first) {
        out += ',';
      }
      first = false;
      writeString(out, member.first);
      out += ':';
      write(out, member.second);
    }
    out += '}';
  }
}

}  // namespace

Value parse(std::string_view text) { return Parser{text}.parseDocument(); }

std::string serialize(const Value& value) {
  std::string out;
  write(out, value);
  return out;
}

}  // namespace glimmer::json
