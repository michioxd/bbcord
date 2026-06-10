#include "../RestClient.hpp"

void DiscordRestClient::downloadAvatar(const QString &userId,
                                       const QString &avatarHash,
                                       const QString &outputPath) {
  RestRequest request;
  request.avatarUserId = userId.trimmed();
  request.avatarHash = avatarHash.trimmed();
  request.outputPath = outputPath.trimmed();
  if (request.avatarUserId.isEmpty() || request.avatarHash.isEmpty() ||
      request.outputPath.isEmpty()) {
    emit avatarDownloadFailed(request.avatarUserId, "Avatar request is empty");
    return;
  }

  request.type = AvatarRequest;
  enqueueRequest(request);
}

void DiscordRestClient::downloadGuildIcon(const QString &guildId,
                                          const QString &iconHash,
                                          const QString &outputPath) {
  RestRequest request;
  request.iconGuildId = guildId.trimmed();
  request.iconHash = iconHash.trimmed();
  request.outputPath = outputPath.trimmed();
  if (request.iconGuildId.isEmpty() || request.iconHash.isEmpty() ||
      request.outputPath.isEmpty()) {
    emit guildIconDownloadFailed(request.iconGuildId,
                                 "Guild icon request is empty");
    return;
  }

  request.type = GuildIconRequest;
  enqueueRequest(request);
}
