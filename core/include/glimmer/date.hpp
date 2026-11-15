#pragma once

/// @file date.hpp
/// Calendar-date helpers built on C++20 <chrono>.
///
/// Diary entries belong to a *calendar day*, not an instant in time, so we use
/// std::chrono::year_month_day everywhere and store it as an ISO-8601 string
/// ("2026-11-14") in the database. That string sorts correctly as text.

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace glimmer {

using Date = std::chrono::year_month_day;

/// Formats a date as "YYYY-MM-DD".
[[nodiscard]] std::string toIsoString(Date date);

/// Parses "YYYY-MM-DD". Returns std::nullopt for malformed or impossible
/// dates (e.g. "2026-02-30").
[[nodiscard]] std::optional<Date> parseIsoDate(std::string_view text);

/// Returns `date` shifted by `days` (may be negative).
[[nodiscard]] Date addDays(Date date, int days);

/// Returns the Monday on or before `date` (ISO-8601 weeks start on Monday).
[[nodiscard]] Date startOfWeek(Date date);

/// First and last day of the month containing `date`.
[[nodiscard]] Date firstDayOfMonth(Date date);
[[nodiscard]] Date lastDayOfMonth(Date date);

}  // namespace glimmer
