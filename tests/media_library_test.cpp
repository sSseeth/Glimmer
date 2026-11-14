#include "glimmer/media_library.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using glimmer::MediaError;
using glimmer::MediaLibrary;

namespace {

class MediaLibraryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    base_ = fs::temp_directory_path() / (std::string{"glimmer-media-"} + info->name());
    fs::remove_all(base_);
    fs::create_directories(base_);
  }
  void TearDown() override { fs::remove_all(base_); }

  fs::path writeFile(const std::string& name, const std::string& content) {
    const auto path = base_ / name;
    std::ofstream(path, std::ios::binary) << content;
    return path;
  }

  fs::path base_;
};

}  // namespace

TEST_F(MediaLibraryTest, CreatesFolders) {
  const MediaLibrary library{base_ / "media"};
  EXPECT_TRUE(fs::is_directory(library.root() / "images"));
  EXPECT_TRUE(fs::is_directory(library.root() / "audio"));
}

TEST_F(MediaLibraryTest, ImportCopiesWithSafeUniqueNames) {
  MediaLibrary library{base_ / "media"};
  const auto source = writeFile("Holiday Photo.JPG", "pixels");

  const auto a = library.importImage(source);
  const auto b = library.importImage(source);
  EXPECT_NE(a, b);
  EXPECT_EQ(a.rfind("images/", 0), 0U);
  EXPECT_EQ(fs::path{a}.extension(), ".jpg");

  std::ifstream copied(library.resolve(a));
  std::string content;
  copied >> content;
  EXPECT_EQ(content, "pixels");
  EXPECT_TRUE(fs::exists(source));  // original untouched
}

TEST_F(MediaLibraryTest, ImportRejectsMissingFiles) {
  MediaLibrary library{base_ / "media"};
  EXPECT_THROW((void)library.importImage(base_ / "nope.png"), MediaError);
}

TEST_F(MediaLibraryTest, NewAudioPathIsInAudioFolder) {
  MediaLibrary library{base_ / "media"};
  const auto path = library.newAudioPath(".wav");
  EXPECT_EQ(path.rfind("audio/", 0), 0U);
  EXPECT_EQ(fs::path{path}.extension(), ".wav");
  EXPECT_FALSE(fs::exists(library.resolve(path)));
}

TEST_F(MediaLibraryTest, ResolveBlocksPathTraversal) {
  const MediaLibrary library{base_ / "media"};
  EXPECT_THROW((void)library.resolve("../outside.png"), MediaError);
  EXPECT_THROW((void)library.resolve("images/../../outside.png"), MediaError);
  EXPECT_THROW((void)library.resolve("/etc/passwd"), MediaError);
  EXPECT_THROW((void)library.resolve(""), MediaError);
  EXPECT_NO_THROW((void)library.resolve("images/./ok.png"));
}

TEST_F(MediaLibraryTest, RemoveDeletesStoredFile) {
  MediaLibrary library{base_ / "media"};
  const auto stored = library.importImage(writeFile("x.png", "data"));
  EXPECT_TRUE(library.remove(stored));
  EXPECT_FALSE(library.remove(stored));
}
