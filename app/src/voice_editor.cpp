#include "voice_editor.hpp"

#include "glimmer/audio.hpp"

#include "qt_convert.hpp"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioSource>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

/// Converts whatever the microphone produced into interleaved int16.
std::vector<std::int16_t> toInt16(const QByteArray& bytes, const QAudioFormat& format) {
  std::vector<std::int16_t> out;
  const auto* data = bytes.constData();
  const auto size = static_cast<std::size_t>(bytes.size());
  switch (format.sampleFormat()) {
    case QAudioFormat::Int16: {
      out.resize(size / sizeof(std::int16_t));
      std::memcpy(out.data(), data, out.size() * sizeof(std::int16_t));
      break;
    }
    case QAudioFormat::Int32: {
      const std::size_t n = size / sizeof(std::int32_t);
      out.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        std::int32_t v = 0;
        std::memcpy(&v, data + i * sizeof(v), sizeof(v));
        out.push_back(static_cast<std::int16_t>(v >> 16));
      }
      break;
    }
    case QAudioFormat::Float: {
      const std::size_t n = size / sizeof(float);
      out.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        float v = 0.0F;
        std::memcpy(&v, data + i * sizeof(v), sizeof(v));
        const float clamped = std::clamp(v, -1.0F, 1.0F);
        out.push_back(static_cast<std::int16_t>(std::lround(clamped * 32767.0F)));
      }
      break;
    }
    case QAudioFormat::UInt8: {
      out.reserve(size);
      for (std::size_t i = 0; i < size; ++i) {
        const int v = static_cast<unsigned char>(data[i]) - 128;
        out.push_back(static_cast<std::int16_t>(v * 256));
      }
      break;
    }
    default:
      break;
  }
  return out;
}

QString formatDuration(qint64 ms) {
  const qint64 seconds = ms / 1000;
  return QStringLiteral("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

}  // namespace

VoiceEditor::VoiceEditor(glimmer::MediaLibrary& media, TranscriberProvider provider,
                         QWidget* parent)
    : EntryEditor(tr("Voice entry"), parent),
      media_(media),
      provider_(std::move(provider)),
      record_(new QPushButton(this)),
      play_(new QPushButton(this)),
      transcribe_(new QPushButton(tr("Transcribe"), this)),
      status_(new QLabel(this)),
      transcript_(new QPlainTextEdit(this)),
      tick_(new QTimer(this)),
      player_(new QMediaPlayer(this)),
      output_(new QAudioOutput(this)),
      watcher_(new QFutureWatcher<TranscriptionOutcome>(this)) {
  player_->setAudioOutput(output_);
  transcript_->setPlaceholderText(
      glimmer::whisperAvailable()
          ? tr("Record, then press Transcribe, or type notes here.")
          : tr("Transcription isn't included in this build; type notes about the recording here."));
  if (!glimmer::whisperAvailable()) {
    transcribe_->setToolTip(tr("Rebuild with -DGLIMMER_WITH_WHISPER=ON to enable"));
  }

  tick_->setInterval(250);
  connect(tick_, &QTimer::timeout, this, &VoiceEditor::updateUi);
  connect(record_, &QPushButton::clicked, this, &VoiceEditor::toggleRecording);
  connect(play_, &QPushButton::clicked, this, &VoiceEditor::togglePlayback);
  connect(transcribe_, &QPushButton::clicked, this, &VoiceEditor::transcribe);
  connect(player_, &QMediaPlayer::playbackStateChanged, this, &VoiceEditor::updateUi);
  connect(watcher_, &QFutureWatcher<TranscriptionOutcome>::finished, this,
          &VoiceEditor::onTranscriptionFinished);

  auto* controls = new QHBoxLayout;
  controls->addWidget(record_);
  controls->addWidget(play_);
  controls->addWidget(transcribe_);
  controls->addStretch();
  controls->addWidget(status_);

  contentLayout()->addLayout(controls);
  contentLayout()->addWidget(transcript_, 1);
  updateUi();
}

VoiceEditor::~VoiceEditor() {
  if (source_ != nullptr) {
    source_->stop();
  }
}

void VoiceEditor::loadEntry(const glimmer::Entry& entry) {
  if (source_ != nullptr) {
    stopRecording();
  }
  player_->stop();
  player_->setSource(QUrl());
  // Switching away from an unsaved recording: clean it up, unless the entry
  // being loaded is the one that just saved it.
  if (audioPath_ != glimmer::qt::toQString(entry.audioPath)) {
    discardUnsavedRecording();
  }
  EntryEditor::loadEntry(entry);
  transcript_->setPlainText(glimmer::qt::toQString(entry.body));
  audioPath_ = glimmer::qt::toQString(entry.audioPath);
  savedAudioPath_ = audioPath_;
  updateUi();
}

void VoiceEditor::writeTo(glimmer::Entry& entry) const {
  EntryEditor::writeTo(entry);
  entry.body = glimmer::qt::toStd(transcript_->toPlainText());
  entry.audioPath = glimmer::qt::toStd(audioPath_);
}

void VoiceEditor::toggleRecording() {
  if (source_ != nullptr) {
    stopRecording();
  } else {
    startRecording();
  }
}

void VoiceEditor::startRecording() {
  const QAudioDevice device = QMediaDevices::defaultAudioInput();
  if (device.isNull()) {
    status_->setText(tr("No microphone found"));
    return;
  }
  // Ask for exactly what whisper wants; fall back to the device's preference
  // and convert afterwards.
  QAudioFormat wanted;
  wanted.setSampleRate(glimmer::audio::kWhisperSampleRate);
  wanted.setChannelCount(1);
  wanted.setSampleFormat(QAudioFormat::Int16);
  format_ = device.isFormatSupported(wanted) ? wanted : device.preferredFormat();

  player_->stop();
  pcm_.clear();
  source_ = new QAudioSource(device, format_, this);
  input_ = source_->start();
  if (input_ == nullptr) {
    status_->setText(tr("Could not start recording"));
    source_->deleteLater();
    source_ = nullptr;
    return;
  }
  connect(input_, &QIODevice::readyRead, this, [this] {
    if (input_ != nullptr) {
      pcm_.append(input_->readAll());
    }
  });
  elapsed_.start();
  tick_->start();
  updateUi();
}

void VoiceEditor::stopRecording() {
  if (source_ == nullptr) {
    return;
  }
  if (input_ != nullptr) {
    pcm_.append(input_->readAll());
  }
  source_->stop();
  source_->deleteLater();
  source_ = nullptr;
  input_ = nullptr;
  tick_->stop();

  glimmer::audio::WavData wav;
  wav.sampleRate = format_.sampleRate();
  wav.channels = format_.channelCount();
  wav.samples = toInt16(pcm_, format_);
  pcm_.clear();
  if (wav.samples.empty()) {
    status_->setText(tr("Nothing was recorded"));
    updateUi();
    return;
  }

  try {
    discardUnsavedRecording();
    const auto relative = media_.newAudioPath(".wav");
    glimmer::audio::writeWav(media_.resolve(relative), wav);
    audioPath_ = glimmer::qt::toQString(relative);
    status_->setText(
        tr("Recorded %1")
            .arg(formatDuration(static_cast<qint64>(wav.samples.size()) * 1000 /
                                (static_cast<qint64>(wav.sampleRate) * wav.channels))));
  } catch (const std::exception& e) {
    status_->setText(tr("Could not save recording: %1").arg(glimmer::qt::toQString(e.what())));
  }
  updateUi();
}

void VoiceEditor::discardUnsavedRecording() {
  // A recording that was never saved to an entry would otherwise be orphaned.
  if (!audioPath_.isEmpty() && audioPath_ != savedAudioPath_) {
    try {
      media_.remove(glimmer::qt::toStd(audioPath_));
    } catch (const std::exception&) {
      // Best effort: a stray file in the media folder is harmless.
    }
  }
}

void VoiceEditor::togglePlayback() {
  if (player_->playbackState() == QMediaPlayer::PlayingState) {
    player_->stop();
    return;
  }
  if (audioPath_.isEmpty()) {
    return;
  }
  try {
    const auto path = media_.resolve(glimmer::qt::toStd(audioPath_));
    player_->setSource(QUrl::fromLocalFile(glimmer::qt::fromPath(path)));
    player_->play();
  } catch (const std::exception& e) {
    status_->setText(glimmer::qt::toQString(e.what()));
  }
}

void VoiceEditor::transcribe() {
  if (audioPath_.isEmpty() || watcher_->isRunning()) {
    return;
  }
  QString error;
  auto transcriber = provider_(&error);
  if (!transcriber) {
    status_->setText(error);
    return;
  }

  std::filesystem::path path;
  try {
    path = media_.resolve(glimmer::qt::toStd(audioPath_));
  } catch (const std::exception& e) {
    status_->setText(glimmer::qt::toQString(e.what()));
    return;
  }

  status_->setText(tr("Transcribing…"));
  // Runs on a worker thread. Captures only values and a shared_ptr, never
  // `this`, so it is safe even if the editor is closed mid-transcription.
  watcher_->setFuture(QtConcurrent::run([transcriber, path]() -> TranscriptionOutcome {
    try {
      const auto wav = glimmer::audio::readWav(path);
      const auto samples =
          glimmer::audio::prepareForWhisper(wav.samples, wav.sampleRate, wav.channels);
      return {true, glimmer::qt::toQString(transcriber->transcribe(samples))};
    } catch (const std::exception& e) {
      return {false, glimmer::qt::toQString(e.what())};
    }
  }));
  updateUi();
}

void VoiceEditor::onTranscriptionFinished() {
  const auto outcome = watcher_->result();
  if (outcome.ok) {
    QString text = transcript_->toPlainText();
    if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n'))) {
      text += QLatin1Char('\n');
    }
    transcript_->setPlainText(text + outcome.text);
    status_->setText(outcome.text.isEmpty() ? tr("No speech detected") : tr("Transcribed"));
  } else {
    status_->setText(tr("Transcription failed: %1").arg(outcome.text));
  }
  updateUi();
}

void VoiceEditor::updateUi() {
  const bool recording = source_ != nullptr;
  const bool playing = player_->playbackState() == QMediaPlayer::PlayingState;
  const bool busy = watcher_->isRunning();
  const bool haveAudio = !audioPath_.isEmpty();

  record_->setText(recording ? tr("■ Stop") : (haveAudio ? tr("● Re-record") : tr("● Record")));
  record_->setEnabled(!busy);
  play_->setText(playing ? tr("■ Stop") : tr("▶ Play"));
  play_->setEnabled(haveAudio && !recording);
  transcribe_->setEnabled(haveAudio && !recording && !busy && glimmer::whisperAvailable());

  if (recording) {
    status_->setText(tr("Recording %1").arg(formatDuration(elapsed_.elapsed())));
  }
}
