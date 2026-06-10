#include "../RestClient.hpp"

void DiscordRestClient::fetchDmChannels(const QString &token, int limit,
                                        const QString &afterId) {
  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  request.requestPath =
      QString("/api/v9/users/@me/channels?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = DmChannelsRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchGuildChannels(const QString &token,
                                           const QString &guildId, int limit,
                                           const QString &afterId) {
  RestRequest request;
  request.token = token.trimmed();
  request.guildId = guildId.trimmed();
  if (request.token.isEmpty() || request.guildId.isEmpty()) {
    emit requestFailed("Guild request is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  request.requestPath = QString("/api/v9/guilds/%1/channels?limit=%2")
                            .arg(request.guildId)
                            .arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = GuildChannelsRequest;
  enqueueRequest(request);
}
