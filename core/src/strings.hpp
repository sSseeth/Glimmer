#pragma once

/// @file strings.hpp
/// Internal string helpers for core/ (not part of the public headers).

#include <string>
#include <string_view>

namespace glimmer::detail {

/// Concatenates strings, string_views and C strings with a single allocation.
///
/// Prefer this to chains like `"text " + std::to_string(n) + "..."`. Besides
/// avoiding temporaries, it sidesteps a GCC 12 false-positive -Wrestrict
/// warning (GCC bug 105329) that fires on `"literal" + std::string&&` in
/// optimised builds.
template <typename... Parts>
[[nodiscard]] std::string concat(const Parts&... parts) {
  std::string out;
  out.reserve((std::string_view{parts}.size() + ... + 0));
  (out.append(std::string_view{parts}), ...);
  return out;
}

}  // namespace glimmer::detail
