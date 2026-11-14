// Compiled when GLIMMER_WITH_WHISPER=OFF: voice entries still record audio,
// they just aren't transcribed.
#include "glimmer/transcriber.hpp"

namespace glimmer {

bool whisperAvailable() noexcept { return false; }

std::unique_ptr<Transcriber> makeWhisperTranscriber(const std::filesystem::path& /*modelPath*/,
                                                    int /*threads*/) {
  throw TranscriptionError(
      "this build of glimmer has no transcription support "
      "(rebuild with -DGLIMMER_WITH_WHISPER=ON)");
}

}  // namespace glimmer
