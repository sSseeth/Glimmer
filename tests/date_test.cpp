#include "glimmer/date.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using glimmer::Date;

TEST(Date, FormatsAsIso8601) {
  EXPECT_EQ(glimmer::toIsoString(Date{year{2026}, November, day{14}}), "2026-11-14");
  EXPECT_EQ(glimmer::toIsoString(Date{year{987}, January, day{5}}), "0987-01-05");
}

TEST(Date, ParsesValidDates) {
  const auto parsed = glimmer::parseIsoDate("2024-02-29");
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(*parsed, (Date{year{2024}, February, day{29}}));
}

TEST(Date, RejectsMalformedOrImpossibleDates) {
  EXPECT_FALSE(glimmer::parseIsoDate(""));
  EXPECT_FALSE(glimmer::parseIsoDate("2026-1-14"));
  EXPECT_FALSE(glimmer::parseIsoDate("2026/11/14"));
  EXPECT_FALSE(glimmer::parseIsoDate("2026-13-01"));
  EXPECT_FALSE(glimmer::parseIsoDate("2025-02-29"));  // not a leap year
  EXPECT_FALSE(glimmer::parseIsoDate("2026-11-1x"));
  EXPECT_FALSE(glimmer::parseIsoDate("+026-11-14"));
}

TEST(Date, RoundTrips) {
  const Date d{year{2026}, December, day{31}};
  EXPECT_EQ(glimmer::parseIsoDate(glimmer::toIsoString(d)), d);
}

TEST(Date, AddDaysCrossesMonthAndYear) {
  EXPECT_EQ(glimmer::addDays(Date{year{2026}, December, day{31}}, 1),
            (Date{year{2027}, January, day{1}}));
  EXPECT_EQ(glimmer::addDays(Date{year{2026}, March, day{1}}, -1),
            (Date{year{2026}, February, day{28}}));
}

TEST(Date, StartOfWeekIsMonday) {
  // 2026-11-14 is a Saturday; the Monday before is 2026-11-09.
  EXPECT_EQ(glimmer::startOfWeek(Date{year{2026}, November, day{14}}),
            (Date{year{2026}, November, day{9}}));
  // A Monday maps to itself.
  EXPECT_EQ(glimmer::startOfWeek(Date{year{2026}, November, day{9}}),
            (Date{year{2026}, November, day{9}}));
  // Sunday belongs to the week that started six days earlier.
  EXPECT_EQ(glimmer::startOfWeek(Date{year{2026}, November, day{15}}),
            (Date{year{2026}, November, day{9}}));
}

TEST(Date, MonthBounds) {
  const Date d{year{2024}, February, day{10}};
  EXPECT_EQ(glimmer::firstDayOfMonth(d), (Date{year{2024}, February, day{1}}));
  EXPECT_EQ(glimmer::lastDayOfMonth(d), (Date{year{2024}, February, day{29}}));
}
