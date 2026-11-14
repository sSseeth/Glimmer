#include "glimmer/date.hpp"

#include <array>
#include <charconv>
#include <cstdio>

namespace glimmer {

namespace chr = std::chrono;

std::string toIsoString(Date date) {
  std::array<char, 16> buffer{};
  const int written =
      std::snprintf(buffer.data(), buffer.size(), "%04d-%02u-%02u", static_cast<int>(date.year()),
                    static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()));
  return {buffer.data(), static_cast<std::size_t>(written)};
}

namespace {

/// Parses exactly `text.size()` decimal digits; rejects signs and spaces.
template <typename Int>
std::optional<Int> parseDigits(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  for (const char c : text) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
  }
  Int value{};
  const auto* first = text.data();
  const auto* last = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc{} || ptr != last) {
    return std::nullopt;
  }
  return value;
}

}  // namespace

std::optional<Date> parseIsoDate(std::string_view text) {
  // Expected layout: YYYY-MM-DD (10 characters).
  if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
    return std::nullopt;
  }
  const auto year = parseDigits<int>(text.substr(0, 4));
  const auto month = parseDigits<unsigned>(text.substr(5, 2));
  const auto day = parseDigits<unsigned>(text.substr(8, 2));
  if (!year || !month || !day) {
    return std::nullopt;
  }
  const Date date{chr::year{*year}, chr::month{*month}, chr::day{*day}};
  if (!date.ok()) {
    return std::nullopt;
  }
  return date;
}

Date addDays(Date date, int days) { return Date{chr::sys_days{date} + chr::days{days}}; }

Date startOfWeek(Date date) {
  const chr::sys_days sd{date};
  const chr::weekday wd{sd};
  // iso_encoding(): Monday = 1 ... Sunday = 7
  const auto offset = static_cast<int>(wd.iso_encoding()) - 1;
  return Date{sd - chr::days{offset}};
}

Date firstDayOfMonth(Date date) { return Date{date.year(), date.month(), chr::day{1}}; }

Date lastDayOfMonth(Date date) {
  const chr::year_month_day_last last{date.year(), chr::month_day_last{date.month()}};
  return Date{last};
}

}  // namespace glimmer
