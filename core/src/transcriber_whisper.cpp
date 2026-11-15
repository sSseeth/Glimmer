// Compiled only when GLIMMER_WITH_WHISPER=ON.
#include "glimmer/transcriber.hpp"

#include <whisper.h>

#include <limits>

namespace glimmer {

namespace {

class WhisperTranscriber final : public Transcriber {
 public:
  WhisperTranscriber(const std::filesystem::path& modelPath, int threads) : threads_(threads) {
    const auto params = whisper_context_default_params();
    ctx_.reset(whisper_init_from_file_with_params(modelPath.string().c_str(), params));
    if (!ctx_) {
      throw TranscriptionError("failed to load whisper model " + modelPath.string());
    }
  }

  std::string transcribe(std::span<const float> samples) override {
    if (samples.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      throw TranscriptionError("recording too long to transcribe");
    }
    auto params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = threads_;
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.language = "auto";

    if (whisper_full(ctx_.get(), params, samples.data(), static_cast<int>(samples.size())) != 0) {
      throw TranscriptionError("whisper failed to transcribe the recording");
    }

    std::string text;
    const int segments = whisper_full_n_segments(ctx_.get());
    for (int i = 0; i < segments; ++i) {
      const char* segment = whisper_full_get_segment_text(ctx_.get(), i);
      if (segment != nullptr) {
        text += segment;
      }
    }
    // Whisper segments usually start with a space.
    const auto first = text.find_first_not_of(' ');
    return first == std::string::npos ? std::string{} : text.substr(first);
  }

 private:
  struct Free {
    void operator()(whisper_context* ctx) const noexcept { whisper_free(ctx); }
  };
  std::unique_ptr<whisper_context, Free> ctx_;
  int threads_;
};

}  // namespace

bool whisperAvailable() noexcept { return true; }

std::unique_ptr<Transcriber> makeWhisperTranscriber(const std::filesystem::path& modelPath,
                                                    int threads) {
  return std::make_unique<WhisperTranscriber>(modelPath, threads);
}

}  // namespace glimmer
