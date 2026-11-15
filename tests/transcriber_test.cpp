#include "glimmer/transcriber.hpp"

#include <gtest/gtest.h>

TEST(Transcriber, MissingModelIsReportedNotCrashed) {
  // Whether or not whisper is compiled in, a missing model must throw a
  // TranscriptionError the UI can show, never crash.
  EXPECT_THROW((void)glimmer::makeWhisperTranscriber("/nonexistent/ggml-model.bin"),
               glimmer::TranscriptionError);
}

/// Example of testing against the interface with a fake, the pattern the app
/// layer's tests would use too.
class FakeTranscriber final : public glimmer::Transcriber {
 public:
  std::string transcribe(std::span<const float> samples) override {
    return "heard " + std::to_string(samples.size()) + " samples";
  }
};

TEST(Transcriber, InterfaceCanBeFaked) {
  FakeTranscriber fake;
  glimmer::Transcriber& transcriber = fake;
  const std::vector<float> silence(16'000, 0.0F);
  EXPECT_EQ(transcriber.transcribe(silence), "heard 16000 samples");
}
