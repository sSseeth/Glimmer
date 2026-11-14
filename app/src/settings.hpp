#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

/// Typed access to QSettings so keys are spelled in exactly one place.
///
/// The API key can also come from the GLIMMER_AI_API_KEY environment
/// variable, which takes precedence and is never written to disk.
class AppSettings {
 public:
  static constexpr const char* kDefaultBaseUrl = "http://127.0.0.1:20128/v1";
  static constexpr const char* kDefaultModel = "auto";

  [[nodiscard]] QString aiBaseUrl() const;
  void setAiBaseUrl(const QString& url);

  [[nodiscard]] QString aiModel() const;
  void setAiModel(const QString& model);

  [[nodiscard]] QString aiApiKey() const;  ///< Env var wins over stored value.
  [[nodiscard]] bool aiApiKeyFromEnvironment() const;
  void setAiApiKey(const QString& key);

  [[nodiscard]] QString whisperModelPath() const;
  void setWhisperModelPath(const QString& path);
};

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  void accept() override;

 private:
  AppSettings settings_;
  QLineEdit* baseUrl_ = nullptr;
  QLineEdit* model_ = nullptr;
  QLineEdit* apiKey_ = nullptr;
  QLineEdit* whisperModel_ = nullptr;
};
