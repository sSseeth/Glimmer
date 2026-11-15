#pragma once

/// @file transcriber.hpp
/// Speech-to-text behind an interface, so the app (and tests) don't care
/// whether whisper.cpp was compiled in.

#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

namespace glimmer {

class TranscriptionError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class Transcriber {
 public:
  Transcriber() = default;
  Transcriber(const Transcriber&) = delete;
  Transcriber& operator=(const Transcriber&) = delete;
  Transcriber(Transcriber&&) = delete;
  Transcriber& operator=(Transcriber&&) = delete;
  virtual ~Transcriber() = default;

  /// `monoSamples16k` is mono float PCM at 16 kHz (see audio::prepareForWhisper).
  /// May take several seconds; call it off the UI thread.
  [[nodiscard]] virtual std::string transcribe(std::span<const float> monoSamples16k) = 0;
};

/// True if this build includes whisper.cpp (GLIMMER_WITH_WHISPER=ON).
[[nodiscard]] bool whisperAvailable() noexcept;

/// Loads a ggml whisper model, e.g. ggml-base.en.bin.
/// Throws TranscriptionError if whisper is unavailable or the model fails to load.
[[nodiscard]] std::unique_ptr<Transcriber> makeWhisperTranscriber(
    const std::filesystem::path& modelPath, int threads = 4);

}  // namespace glimmer
