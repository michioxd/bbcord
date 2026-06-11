#include "../RestClient.hpp"

void DiscordRestClient::fetchDmChannels(const QString &token, int limit,
                                        const QString &afterId) {
  Q_UNUSED(limit);
  Q_UNUSED(afterId);

  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  request.requestPath = "/api/v9/users/@me/channels";

  request.type = DmChannelsRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchGuildChannels(const QString &token,
                                           const QString &guildId, int limit,
                                           const QString &afterId) {
  Q_UNUSED(limit);
  Q_UNUSED(afterId);

  RestRequest request;
  request.token = token.trimmed();
  request.guildId = guildId.trimmed();
  if (request.token.isEmpty() || request.guildId.isEmpty()) {
    emit requestFailed("Guild request is empty");
    return;
  }

  request.requestPath =
      QString("/api/v9/guilds/%1/channels").arg(request.guildId);

  request.type = GuildChannelsRequest;
  enqueueRequest(request);
}
