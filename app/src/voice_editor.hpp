#pragma once

#include "glimmer/media_library.hpp"
#include "glimmer/transcriber.hpp"

#include "entry_editor.hpp"

#include <QAudioFormat>
#include <QByteArray>
#include <QElapsedTimer>
#include <QFutureWatcher>

#include <functional>
#include <memory>

class QAudioOutput;
class QAudioSource;
class QIODevice;
class QLabel;
class QMediaPlayer;
class QPlainTextEdit;
class QPushButton;
class QTimer;

/// Result of a background transcription job.
struct TranscriptionOutcome {
  bool ok = false;
  QString text;  ///< Transcript on success, error message on failure.
};

/// Record from the microphone, play it back, and (if whisper.cpp is built in)
/// transcribe it locally. Audio is stored as 16-bit PCM WAV in the media
/// library; the transcript is the entry body and can be edited by hand.
class VoiceEditor : public EntryEditor {
  Q_OBJECT

 public:
  /// Returns a ready transcriber, or nullptr and sets *error.
  using TranscriberProvider = std::function<std::shared_ptr<glimmer::Transcriber>(QString* error)>;

  VoiceEditor(glimmer::MediaLibrary& media, TranscriberProvider provider,
              QWidget* parent = nullptr);
  ~VoiceEditor() override;

  VoiceEditor(const VoiceEditor&) = delete;
  VoiceEditor& operator=(const VoiceEditor&) = delete;
  VoiceEditor(VoiceEditor&&) = delete;
  VoiceEditor& operator=(VoiceEditor&&) = delete;

  void loadEntry(const glimmer::Entry& entry) override;
  void writeTo(glimmer::Entry& entry) const override;

 private:
  void toggleRecording();
  void startRecording();
  void stopRecording();
  void togglePlayback();
  void transcribe();
  void onTranscriptionFinished();
  void discardUnsavedRecording();
  void updateUi();

  glimmer::MediaLibrary& media_;
  TranscriberProvider provider_;

  QPushButton* record_;
  QPushButton* play_;
  QPushButton* transcribe_;
  QLabel* status_;
  QPlainTextEdit* transcript_;
  QTimer* tick_;
  QMediaPlayer* player_;
  QAudioOutput* output_;
  QFutureWatcher<TranscriptionOutcome>* watcher_;

  QAudioSource* source_ = nullptr;  // Non-null only while recording.
  QIODevice* input_ = nullptr;
  QAudioFormat format_;
  QByteArray pcm_;
  QElapsedTimer elapsed_;

  QString audioPath_;       ///< Relative path of the current recording.
  QString savedAudioPath_;  ///< Relative path as last loaded from the database.
};
