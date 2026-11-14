#pragma once

/// @file media_library.hpp
/// Owns the folder where images and recordings live.
///
/// Files are copied *into* the library when attached to an entry, so the
/// diary keeps working if the original is moved or deleted. The database
/// stores paths relative to the library root, so the whole data folder can be
/// backed up or moved as one unit.

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace glimmer {

class MediaError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class MediaLibrary {
 public:
  /// Creates `root` (and the images/ and audio/ subfolders) if needed.
  explicit MediaLibrary(std::filesystem::path root);

  [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }

  /// Copies `source` into images/ and returns the new *relative* path.
  [[nodiscard]] std::string importImage(const std::filesystem::path& source);

  /// Reserves a fresh relative path in audio/ with the given extension
  /// (e.g. ".wav"). The file is not created.
  [[nodiscard]] std::string newAudioPath(std::string_view extension);

  /// Turns a stored relative path into an absolute one. Throws MediaError if
  /// the path tries to escape the library (e.g. "../../etc/passwd").
  [[nodiscard]] std::filesystem::path resolve(std::string_view relativePath) const;

  /// Deletes a stored file if it exists. Returns true if something was removed.
  bool remove(std::string_view relativePath) const;

 private:
  std::string uniqueName(std::string_view folder, std::string_view extension) const;

  std::filesystem::path root_;
};

}  // namespace glimmer
