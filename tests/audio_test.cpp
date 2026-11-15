#include "glimmer/audio.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace audio = glimmer::audio;
namespace fs = std::filesystem;

namespace {

/// A unique temp file that deletes itself.
class TempFile {
 public:
  explicit TempFile(const std::string& name)
      : path_(fs::temp_directory_path() /
              (name + "-" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()))) {}
  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;
  TempFile(TempFile&&) = delete;
  TempFile& operator=(TempFile&&) = delete;
  ~TempFile() {
    std::error_code ec;
    fs::remove(path_, ec);
  }
  [[nodiscard]] const fs::path& path() const { return path_; }

 private:
  fs::path path_;
};

}  // namespace

TEST(Audio, Int16ToFloatScales) {
  const std::vector<std::int16_t> in{0, 16384, -32768, 32767};
  const auto out = audio::int16ToFloat(in);
  ASSERT_EQ(out.size(), 4U);
  EXPECT_FLOAT_EQ(out[0], 0.0F);
  EXPECT_FLOAT_EQ(out[1], 0.5F);
  EXPECT_FLOAT_EQ(out[2], -1.0F);
  EXPECT_LT(out[3], 1.0F);
}

TEST(Audio, DownmixAveragesChannels) {
  const std::vector<float> stereo{1.0F, 0.0F, 0.5F, 0.5F, -1.0F, 1.0F};
  const auto mono = audio::downmixToMono(stereo, 2);
  ASSERT_EQ(mono.size(), 3U);
  EXPECT_FLOAT_EQ(mono[0], 0.5F);
  EXPECT_FLOAT_EQ(mono[1], 0.5F);
  EXPECT_FLOAT_EQ(mono[2], 0.0F);
  EXPECT_THROW((void)audio::downmixToMono(stereo, 0), audio::AudioError);
}

TEST(Audio, ResampleChangesLengthProportionally) {
  const std::vector<float> oneSecondAt48k(48'000, 0.25F);
  const auto out = audio::resampleLinear(oneSecondAt48k, 48'000, 16'000);
  EXPECT_EQ(out.size(), 16'000U);
  EXPECT_FLOAT_EQ(out.front(), 0.25F);
  EXPECT_FLOAT_EQ(out.back(), 0.25F);
}

TEST(Audio, ResampleInterpolatesBetweenSamples) {
  const std::vector<float> ramp{0.0F, 1.0F, 2.0F, 3.0F};
  const auto up = audio::resampleLinear(ramp, 1, 2);  // double the rate
  ASSERT_EQ(up.size(), 8U);
  EXPECT_FLOAT_EQ(up[1], 0.5F);
  EXPECT_FLOAT_EQ(up[2], 1.0F);
  EXPECT_FLOAT_EQ(up[3], 1.5F);
}

TEST(Audio, PrepareForWhisperProducesMono16k) {
  const std::vector<std::int16_t> stereo44k(44'100 * 2, 1000);
  const auto out = audio::prepareForWhisper(stereo44k, 44'100, 2);
  EXPECT_EQ(out.size(), 16'000U);
}

TEST(Wav, WriteThenReadRoundTrips) {
  const TempFile file{"glimmer-roundtrip.wav"};
  audio::WavData data;
  data.sampleRate = 16'000;
  data.channels = 1;
  data.samples = {0, 1, -1, 32767, -32768, 1234};
  audio::writeWav(file.path(), data);

  EXPECT_EQ(fs::file_size(file.path()), 44U + data.samples.size() * 2U);
  const auto read = audio::readWav(file.path());
  EXPECT_EQ(read.sampleRate, 16'000);
  EXPECT_EQ(read.channels, 1);
  EXPECT_EQ(read.samples, data.samples);
}

TEST(Wav, RejectsNonWavFiles) {
  const TempFile file{"glimmer-not-a.wav"};
  {
    std::ofstream out(file.path(), std::ios::binary);
    out << "definitely not riff data";
  }
  EXPECT_THROW((void)audio::readWav(file.path()), audio::AudioError);
  EXPECT_THROW((void)audio::readWav("/nonexistent/glimmer.wav"), audio::AudioError);
}
