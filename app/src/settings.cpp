#include "settings.hpp"

#include "glimmer/transcriber.hpp"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace {

constexpr auto kBaseUrlKey = "ai/baseUrl";
constexpr auto kModelKey = "ai/model";
constexpr auto kApiKeyKey = "ai/apiKey";
constexpr auto kWhisperKey = "voice/whisperModelPath";
constexpr auto kApiKeyEnv = "GLIMMER_AI_API_KEY";

}  // namespace

QString AppSettings::aiBaseUrl() const {
  return QSettings().value(kBaseUrlKey, QString::fromLatin1(kDefaultBaseUrl)).toString();
}

void AppSettings::setAiBaseUrl(const QString& url) {
  QSettings().setValue(kBaseUrlKey, url.trimmed());
}

QString AppSettings::aiModel() const {
  return QSettings().value(kModelKey, QString::fromLatin1(kDefaultModel)).toString();
}

void AppSettings::setAiModel(const QString& model) {
  QSettings().setValue(kModelKey, model.trimmed());
}

bool AppSettings::aiApiKeyFromEnvironment() const {
  return !qEnvironmentVariableIsEmpty(kApiKeyEnv);
}

QString AppSettings::aiApiKey() const {
  if (aiApiKeyFromEnvironment()) {
    return qEnvironmentVariable(kApiKeyEnv);
  }
  return QSettings().value(kApiKeyKey).toString();
}

void AppSettings::setAiApiKey(const QString& key) {
  if (key.isEmpty()) {
    QSettings().remove(kApiKeyKey);
  } else {
    QSettings().setValue(kApiKeyKey, key);
  }
}

QString AppSettings::whisperModelPath() const { return QSettings().value(kWhisperKey).toString(); }

void AppSettings::setWhisperModelPath(const QString& path) {
  QSettings().setValue(kWhisperKey, path);
}

// --- SettingsDialog ----------------------------------------------------------

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(tr("Settings"));

  baseUrl_ = new QLineEdit(settings_.aiBaseUrl(), this);
  baseUrl_->setPlaceholderText(QString::fromLatin1(AppSettings::kDefaultBaseUrl));
  model_ = new QLineEdit(settings_.aiModel(), this);
  apiKey_ = new QLineEdit(this);
  apiKey_->setEchoMode(QLineEdit::Password);
  if (settings_.aiApiKeyFromEnvironment()) {
    apiKey_->setPlaceholderText(tr("Set by GLIMMER_AI_API_KEY"));
    apiKey_->setEnabled(false);
  } else {
    apiKey_->setText(settings_.aiApiKey());
    apiKey_->setPlaceholderText(tr("Endpoint key created in your gateway"));
  }

  whisperModel_ = new QLineEdit(settings_.whisperModelPath(), this);
  auto* browse = new QPushButton(tr("Browse…"), this);
  connect(browse, &QPushButton::clicked, this, [this] {
    const auto path = QFileDialog::getOpenFileName(this, tr("Choose a whisper model"), QString(),
                                                   tr("ggml models (*.bin);;All files (*)"));
    if (!path.isEmpty()) {
      whisperModel_->setText(path);
    }
  });
  auto* whisperRow = new QHBoxLayout;
  whisperRow->addWidget(whisperModel_);
  whisperRow->addWidget(browse);

  auto* form = new QFormLayout;
  form->addRow(tr("AI gateway URL"), baseUrl_);
  form->addRow(tr("Model"), model_);
  form->addRow(tr("API key"), apiKey_);
  form->addRow(tr("Whisper model"), whisperRow);

  auto* note = new QLabel(
      tr("The AI panel sends the entries it summarises to this gateway (for example a local "
         "OmniRoute instance). Nothing is sent until you press a button in the AI panel.\n"
         "Voice transcription runs locally with whisper.cpp%1.")
          .arg(glimmer::whisperAvailable() ? QString() : tr(" (not included in this build)")),
      this);
  note->setWordWrap(true);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(note);
  layout->addWidget(buttons);
  resize(520, sizeHint().height());
}

void SettingsDialog::accept() {
  settings_.setAiBaseUrl(baseUrl_->text().isEmpty()
                             ? QString::fromLatin1(AppSettings::kDefaultBaseUrl)
                             : baseUrl_->text());
  settings_.setAiModel(model_->text().isEmpty() ? QString::fromLatin1(AppSettings::kDefaultModel)
                                                : model_->text());
  if (!settings_.aiApiKeyFromEnvironment()) {
    settings_.setAiApiKey(apiKey_->text());
  }
  settings_.setWhisperModelPath(whisperModel_->text().trimmed());
  QDialog::accept();
}
