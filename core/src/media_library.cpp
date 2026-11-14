#include "glimmer/media_library.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <random>

namespace glimmer {

namespace fs = std::filesystem;

namespace {

/// Keeps only a short, safe, lower-case extension like ".png".
std::string sanitizeExtension(std::string_view extension) {
  std::string out;
  for (const char c : extension) {
    const auto uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc) != 0) {
      out += static_cast<char>(std::tolower(uc));
    }
    if (out.size() >= 8) {
      break;
    }
  }
  return out.empty() ? std::string{} : "." + out;
}

}  // namespace

MediaLibrary::MediaLibrary(fs::path root) : root_(std::move(root)) {
  fs::create_directories(root_ / "images");
  fs::create_directories(root_ / "audio");
  root_ = fs::canonical(root_);
}

std::string MediaLibrary::uniqueName(std::string_view folder, std::string_view extension) const {
  const auto ext = sanitizeExtension(extension);
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::random_device device;
  std::uniform_int_distribution<unsigned> distribution(0, 0xFFFFFF);
  for (int attempt = 0; attempt < 100; ++attempt) {
    const std::string name = std::string{folder} + "/" + std::to_string(millis) + "-" +
                             std::to_string(distribution(device)) + ext;
    if (!fs::exists(root_ / name)) {
      return name;
    }
  }
  throw MediaError("could not find a free file name in " + std::string{folder});
}

std::string MediaLibrary::importImage(const fs::path& source) {
  if (!fs::is_regular_file(source)) {
    throw MediaError("not a file: " + source.string());
  }
  const auto relative = uniqueName("images", source.extension().string());
  fs::copy_file(source, root_ / relative);
  return relative;
}

std::string MediaLibrary::newAudioPath(std::string_view extension) {
  return uniqueName("audio", extension);
}

fs::path MediaLibrary::resolve(std::string_view relativePath) const {
  const fs::path relative{std::string{relativePath}};
  if (relative.empty() || relative.is_absolute()) {
    throw MediaError("media paths must be relative: '" + std::string{relativePath} + "'");
  }
  const auto candidate = (root_ / relative).lexically_normal();
  // The normalized path must still start with the root.
  const auto [rootEnd, candidateIt] =
      std::mismatch(root_.begin(), root_.end(), candidate.begin(), candidate.end());
  if (rootEnd != root_.end()) {
    throw MediaError("media path escapes the library: '" + std::string{relativePath} + "'");
  }
  return candidate;
}

bool MediaLibrary::remove(std::string_view relativePath) const {
  return fs::remove(resolve(relativePath));
}

}  // namespace glimmer
