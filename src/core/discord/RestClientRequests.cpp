#include "RestClient.hpp"

#include "DiscordUtils.hpp"

extern "C" {
#include "mongoose.h"
}

void DiscordRestClient::sendApiRequest(struct mg_connection *connection) {
  if (connection == NULL || m_requestSent || m_requestPath.isEmpty()) {
    return;
  }

  m_requestSent = true;

  QByteArray pathBytes = m_requestPath.toUtf8();
  QByteArray tokenBytes = m_token.toUtf8();
  QByteArray userAgent = DiscordUtils::desktopUserAgent();
  QByteArray methodBytes =
      m_requestMethod.isEmpty() ? QByteArray("GET") : m_requestMethod.toUtf8();
  if (m_requestBody.isEmpty()) {
    mg_printf(connection,
              "%s %s HTTP/1.1\r\n"
              "Host: discord.com\r\n"
              "Authorization: %s\r\n"
              "User-Agent: %s\r\n"
              "Accept: application/json\r\n"
              "Connection: keep-alive\r\n\r\n",
              methodBytes.constData(), pathBytes.constData(),
              tokenBytes.constData(), userAgent.constData());
    return;
  }

  QByteArray contentTypeBytes = m_contentType.isEmpty()
                                    ? QByteArray("application/json")
                                    : m_contentType.toUtf8();
  mg_printf(connection,
            "%s %s HTTP/1.1\r\n"
            "Host: discord.com\r\n"
            "Authorization: %s\r\n"
            "User-Agent: %s\r\n"
            "Accept: application/json\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: keep-alive\r\n\r\n",
            methodBytes.constData(), pathBytes.constData(),
            tokenBytes.constData(), userAgent.constData(),
            contentTypeBytes.constData(),
            static_cast<int>(m_requestBody.size()));
  mg_send(connection, m_requestBody.constData(),
          static_cast<size_t>(m_requestBody.size()));
}

void DiscordRestClient::sendAvatarRequest(struct mg_connection *connection) {
  if (m_requestType != AvatarRequest || connection == NULL || m_requestSent) {
    return;
  }

  m_requestSent = true;

  QByteArray userIdBytes = m_avatarUserId.toUtf8();
  QByteArray avatarHashBytes = m_avatarHash.toUtf8();
  QByteArray userAgent = DiscordUtils::desktopUserAgent();
  mg_printf(connection,
            "GET /avatars/%s/%s.png?size=128 HTTP/1.1\r\n"
            "Host: cdn.discordapp.com\r\n"
            "User-Agent: %s\r\n"
            "Accept: image/png,image/*\r\n"
            "Connection: keep-alive\r\n\r\n",
            userIdBytes.constData(), avatarHashBytes.constData(),
            userAgent.constData());
}

void DiscordRestClient::sendGuildIconRequest(struct mg_connection *connection) {
  if (m_requestType != GuildIconRequest || connection == NULL ||
      m_requestSent) {
    return;
  }

  m_requestSent = true;

  QByteArray guildIdBytes = m_iconGuildId.toUtf8();
  QByteArray iconHashBytes = m_iconHash.toUtf8();
  QByteArray userAgent = DiscordUtils::desktopUserAgent();
  mg_printf(connection,
            "GET /icons/%s/%s.png?size=128 HTTP/1.1\r\n"
            "Host: cdn.discordapp.com\r\n"
            "User-Agent: %s\r\n"
            "Accept: image/png,image/*\r\n"
            "Connection: keep-alive\r\n\r\n",
            guildIdBytes.constData(), iconHashBytes.constData(),
            userAgent.constData());
}
