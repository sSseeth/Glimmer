#include "glimmer/audio.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>

namespace glimmer::audio {

std::vector<float> int16ToFloat(std::span<const std::int16_t> samples) {
  std::vector<float> out;
  out.reserve(samples.size());
  for (const auto s : samples) {
    out.push_back(static_cast<float>(s) / 32768.0F);
  }
  return out;
}

std::vector<float> downmixToMono(std::span<const float> interleaved, int channels) {
  if (channels <= 0) {
    throw AudioError("channel count must be positive");
  }
  const auto n = static_cast<std::size_t>(channels);
  if (n == 1) {
    return {interleaved.begin(), interleaved.end()};
  }
  const std::size_t frames = interleaved.size() / n;
  std::vector<float> out(frames);
  for (std::size_t frame = 0; frame < frames; ++frame) {
    float sum = 0.0F;
    for (std::size_t ch = 0; ch < n; ++ch) {
      sum += interleaved[frame * n + ch];
    }
    out[frame] = sum / static_cast<float>(n);
  }
  return out;
}

std::vector<float> resampleLinear(std::span<const float> samples, int fromRate, int toRate) {
  if (fromRate <= 0 || toRate <= 0) {
    throw AudioError("sample rates must be positive");
  }
  if (fromRate == toRate || samples.empty()) {
    return {samples.begin(), samples.end()};
  }
  const double ratio = static_cast<double>(fromRate) / static_cast<double>(toRate);
  const auto outLength =
      static_cast<std::size_t>(std::floor(static_cast<double>(samples.size()) / ratio));
  std::vector<float> out(outLength);
  const std::size_t last = samples.size() - 1;
  for (std::size_t i = 0; i < outLength; ++i) {
    const double position = static_cast<double>(i) * ratio;
    const auto index = static_cast<std::size_t>(position);
    const double frac = position - static_cast<double>(index);
    const float a = samples[std::min(index, last)];
    const float b = samples[std::min(index + 1, last)];
    out[i] = static_cast<float>(static_cast<double>(a) +
                                (static_cast<double>(b) - static_cast<double>(a)) * frac);
  }
  return out;
}

std::vector<float> prepareForWhisper(std::span<const std::int16_t> interleaved, int sampleRate,
                                     int channels) {
  const auto asFloat = int16ToFloat(interleaved);
  const auto mono = downmixToMono(asFloat, channels);
  return resampleLinear(mono, sampleRate, kWhisperSampleRate);
}

// --- WAV ---------------------------------------------------------------------

namespace {

void putU16(std::ofstream& out, std::uint16_t v) {
  const std::array<char, 2> bytes{static_cast<char>(v & 0xFFU), static_cast<char>(v >> 8U)};
  out.write(bytes.data(), bytes.size());
}

void putU32(std::ofstream& out, std::uint32_t v) {
  const std::array<char, 4> bytes{
      static_cast<char>(v & 0xFFU), static_cast<char>((v >> 8U) & 0xFFU),
      static_cast<char>((v >> 16U) & 0xFFU), static_cast<char>((v >> 24U) & 0xFFU)};
  out.write(bytes.data(), bytes.size());
}

std::uint16_t getU16(const std::vector<unsigned char>& b, std::size_t at) {
  return static_cast<std::uint16_t>(b.at(at) | (b.at(at + 1) << 8U));
}

std::uint32_t getU32(const std::vector<unsigned char>& b, std::size_t at) {
  return static_cast<std::uint32_t>(b.at(at)) | (static_cast<std::uint32_t>(b.at(at + 1)) << 8U) |
         (static_cast<std::uint32_t>(b.at(at + 2)) << 16U) |
         (static_cast<std::uint32_t>(b.at(at + 3)) << 24U);
}

}  // namespace

void writeWav(const std::filesystem::path& path, const WavData& data) {
  if (data.sampleRate <= 0 || data.channels <= 0 || data.channels > 0xFFFF) {
    throw AudioError("invalid WAV format");
  }
  constexpr std::uint16_t kBitsPerSample = 16;
  const auto channels = static_cast<std::uint16_t>(data.channels);
  const auto rate = static_cast<std::uint32_t>(data.sampleRate);
  const auto blockAlign = static_cast<std::uint16_t>(channels * (kBitsPerSample / 8U));
  const std::uint64_t dataBytes64 = data.samples.size() * sizeof(std::int16_t);
  if (dataBytes64 > std::numeric_limits<std::uint32_t>::max() - 36U) {
    throw AudioError("recording too long for a WAV file");
  }
  const auto dataBytes = static_cast<std::uint32_t>(dataBytes64);

  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw AudioError("cannot open " + path.string() + " for writing");
  }
  out.write("RIFF", 4);
  putU32(out, 36U + dataBytes);
  out.write("WAVE", 4);
  out.write("fmt ", 4);
  putU32(out, 16U);  // fmt chunk size
  putU16(out, 1U);   // PCM
  putU16(out, channels);
  putU32(out, rate);
  putU32(out, rate * blockAlign);  // byte rate
  putU16(out, blockAlign);
  putU16(out, kBitsPerSample);
  out.write("data", 4);
  putU32(out, dataBytes);
  for (const auto sample : data.samples) {
    putU16(out, static_cast<std::uint16_t>(sample));
  }
  if (!out) {
    throw AudioError("failed writing " + path.string());
  }
}

WavData readWav(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw AudioError("cannot open " + path.string());
  }
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                         std::istreambuf_iterator<char>());
  auto tag = [&](std::size_t at, const char* expected) {
    return at + 4 <= bytes.size() &&
           std::equal(expected, expected + 4, bytes.begin() + static_cast<std::ptrdiff_t>(at));
  };
  if (!tag(0, "RIFF") || !tag(8, "WAVE")) {
    throw AudioError(path.string() + " is not a WAV file");
  }

  WavData result;
  bool haveFormat = false;
  std::size_t pos = 12;
  while (pos + 8 <= bytes.size()) {
    const std::uint32_t chunkSize = getU32(bytes, pos + 4);
    const std::size_t body = pos + 8;
    if (body + chunkSize > bytes.size()) {
      throw AudioError("truncated WAV chunk");
    }
    if (tag(pos, "fmt ")) {
      if (chunkSize < 16 || getU16(bytes, body) != 1U || getU16(bytes, body + 14) != 16U) {
        throw AudioError("only 16-bit PCM WAV is supported");
      }
      result.channels = getU16(bytes, body + 2);
      result.sampleRate = static_cast<int>(getU32(bytes, body + 4));
      haveFormat = true;
    } else if (tag(pos, "data")) {
      if (!haveFormat) {
        throw AudioError("WAV data chunk before fmt chunk");
      }
      result.samples.resize(chunkSize / 2U);
      for (std::size_t i = 0; i < result.samples.size(); ++i) {
        result.samples[i] = static_cast<std::int16_t>(getU16(bytes, body + i * 2U));
      }
      return result;
    }
    pos = body + chunkSize + (chunkSize % 2U);  // Chunks are word-aligned.
  }
  throw AudioError("WAV file has no data chunk");
}

}  // namespace glimmer::audio
