#pragma once

#include "glimmer/ai.hpp"

#include <QObject>
#include <QString>

#include <functional>

class QNetworkAccessManager;

/// Async client for any OpenAI-compatible chat-completions endpoint.
/// Written with OmniRoute in mind (default http://127.0.0.1:20128/v1), but
/// nothing here is OmniRoute-specific.
///
/// Request/response encoding lives in glimmer::ai (core) and is unit-tested
/// there; this class only does HTTP.
class OmniRouteClient : public QObject {
  Q_OBJECT

 public:
  using SuccessHandler = std::function<void(const glimmer::ai::ChatResult&)>;
  using ErrorHandler = std::function<void(const QString&)>;

  explicit OmniRouteClient(QObject* parent = nullptr);

  /// Sends `request` to `<baseUrl>/chat/completions`. Exactly one of the
  /// handlers is called, on the UI thread.
  void send(const QString& baseUrl, const QString& apiKey, const glimmer::ai::ChatRequest& request,
            SuccessHandler onSuccess, ErrorHandler onError);

 private:
  QNetworkAccessManager* network_;
};
