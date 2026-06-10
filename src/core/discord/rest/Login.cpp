#include "../RestClient.hpp"

#include "../DiscordUtils.hpp"

extern "C" {
#include "mongoose.h"
}

void DiscordRestClient::loginWithToken(const QString &token) {
  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit loginFailed("Token is empty");
    return;
  }

  request.type = LoginRequest;
  enqueueRequest(request);
}

void DiscordRestClient::sendGetMeRequest(struct mg_connection *connection) {
  if (m_requestType != LoginRequest || connection == NULL || m_requestSent) {
    return;
  }

  m_requestSent = true;

  QByteArray tokenBytes = m_token.toUtf8();
  QByteArray userAgent = DiscordUtils::desktopUserAgent();
  mg_printf(connection,
            "GET /api/v9/users/@me HTTP/1.1\r\n"
            "Host: discord.com\r\n"
            "Authorization: %s\r\n"
            "User-Agent: %s\r\n"
            "Accept: application/json\r\n"
            "Connection: keep-alive\r\n\r\n",
            tokenBytes.constData(), userAgent.constData());
}

void DiscordRestClient::succeedWithUser(const QVariantMap &user) {
  if (m_finished) {
    return;
  }

  finishRequest();
  emit loginSucceeded(user);
  processNextRequest();
}
