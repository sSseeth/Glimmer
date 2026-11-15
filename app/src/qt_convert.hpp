#pragma once

/// @file qt_convert.hpp
/// The one place where Qt types meet core types. Core uses std::string
/// (always UTF-8) and std::chrono dates; Qt uses QString and QDate.

#include "glimmer/date.hpp"

#include <QDate>
#include <QString>

#include <filesystem>
#include <string>
#include <string_view>

namespace glimmer::qt {

inline QDate toQDate(Date date) {
  return QDate(static_cast<int>(date.year()), static_cast<int>(static_cast<unsigned>(date.month())),
               static_cast<int>(static_cast<unsigned>(date.day())));
}

inline Date fromQDate(QDate date) {
  return Date{std::chrono::year{date.year()},
              std::chrono::month{static_cast<unsigned>(date.month())},
              std::chrono::day{static_cast<unsigned>(date.day())}};
}

inline QString toQString(std::string_view text) {
  return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

inline std::string toStd(const QString& text) {
  return text.toStdString();  // UTF-8
}

/// UTF-16 round-trip keeps non-ASCII paths correct on Windows too.
inline std::filesystem::path toPath(const QString& path) {
  return std::filesystem::path{path.toStdU16String()};
}

inline QString fromPath(const std::filesystem::path& path) {
  return QString::fromStdU16String(path.u16string());
}

}  // namespace glimmer::qt
