#pragma once

/// @file audio.hpp
/// Audio sample conversion and 16-bit PCM WAV files.
///
/// The microphone gives us interleaved 16-bit samples at whatever rate the
/// device prefers; whisper.cpp wants mono 32-bit float at 16 kHz. These pure
/// functions bridge the two and are easy to unit-test.

#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <vector>

namespace glimmer::audio {

inline constexpr int kWhisperSampleRate = 16'000;

class AudioError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Maps int16 [-32768, 32767] to float [-1, 1).
[[nodiscard]] std::vector<float> int16ToFloat(std::span<const std::int16_t> samples);

/// Averages interleaved channels into one.
[[nodiscard]] std::vector<float> downmixToMono(std::span<const float> interleaved, int channels);

/// Linear-interpolation resampler. Good enough for speech recognition.
[[nodiscard]] std::vector<float> resampleLinear(std::span<const float> samples, int fromRate,
                                                int toRate);

/// Full pipeline: int16 interleaved @ any rate -> float mono @ 16 kHz.
[[nodiscard]] std::vector<float> prepareForWhisper(std::span<const std::int16_t> interleaved,
                                                   int sampleRate, int channels);

struct WavData {
  int sampleRate = 0;
  int channels = 0;
  std::vector<std::int16_t> samples;  ///< Interleaved.
};

/// Writes a canonical 44-byte-header PCM16 WAV file.
void writeWav(const std::filesystem::path& path, const WavData& data);

/// Reads a PCM16 WAV file, skipping unknown chunks. Throws AudioError.
[[nodiscard]] WavData readWav(const std::filesystem::path& path);

}  // namespace glimmer::audio
