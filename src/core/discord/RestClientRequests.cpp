#include "RestClient.hpp"

#include "DiscordUtils.hpp"

extern "C" {
#include "mongoose.h"
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
            "Connection: close\r\n\r\n",
            tokenBytes.constData(), userAgent.constData());
}

void DiscordRestClient::sendApiRequest(struct mg_connection *connection) {
  if (connection == NULL || m_requestSent || m_requestPath.isEmpty()) {
    return;
  }

  m_requestSent = true;

  QByteArray pathBytes = m_requestPath.toUtf8();
  QByteArray tokenBytes = m_token.toUtf8();
  QByteArray userAgent = DiscordUtils::desktopUserAgent();
  mg_printf(connection,
            "GET %s HTTP/1.1\r\n"
            "Host: discord.com\r\n"
            "Authorization: %s\r\n"
            "User-Agent: %s\r\n"
            "Accept: application/json\r\n"
            "Connection: close\r\n\r\n",
            pathBytes.constData(), tokenBytes.constData(),
            userAgent.constData());
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
            "Connection: close\r\n\r\n",
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
            "Connection: close\r\n\r\n",
            guildIdBytes.constData(), iconHashBytes.constData(),
            userAgent.constData());
}
