#include "omniroute_client.hpp"

#include "qt_convert.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

constexpr int kTimeoutMs = 120'000;

}  // namespace

OmniRouteClient::OmniRouteClient(QObject* parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)) {}

void OmniRouteClient::send(const QString& baseUrl, const QString& apiKey,
                           const glimmer::ai::ChatRequest& request, SuccessHandler onSuccess,
                           ErrorHandler onError) {
  QString base = baseUrl.trimmed();
  while (base.endsWith(QLatin1Char('/'))) {
    base.chop(1);
  }
  const QUrl url(base + QStringLiteral("/chat/completions"));
  if (!url.isValid() || url.scheme().isEmpty()) {
    onError(tr("The AI gateway URL \"%1\" is not valid. Check Settings.").arg(baseUrl));
    return;
  }

  QNetworkRequest httpRequest(url);
  httpRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
  httpRequest.setTransferTimeout(kTimeoutMs);
  if (!apiKey.isEmpty()) {
    httpRequest.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
  }

  const auto body = glimmer::ai::encodeChatRequest(request);
  QNetworkReply* reply = network_->post(httpRequest, QByteArray::fromStdString(body));

  connect(reply, &QNetworkReply::finished, this,
          [reply, onSuccess = std::move(onSuccess), onError = std::move(onError)] {
            reply->deleteLater();
            const QByteArray payload = reply->readAll();

            // Transport failure with no body (connection refused, timeout…).
            if (reply->error() != QNetworkReply::NoError && payload.isEmpty()) {
              onError(tr("Could not reach the AI gateway: %1").arg(reply->errorString()));
              return;
            }
            try {
              // decodeChatResponse also turns {"error": {...}} bodies into
              // exceptions with the gateway's own message.
              onSuccess(glimmer::ai::decodeChatResponse(
                  std::string_view(payload.constData(), static_cast<std::size_t>(payload.size()))));
            } catch (const glimmer::ai::AiError& e) {
              onError(glimmer::qt::toQString(e.what()));
            }
          });
}
