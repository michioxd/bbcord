#include "../RestClient.hpp"

void DiscordRestClient::fetchGuilds(const QString &token, int limit,
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

  request.requestPath = QString("/api/v9/users/@me/guilds?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = GuildsRequest;
  enqueueRequest(request);
}
