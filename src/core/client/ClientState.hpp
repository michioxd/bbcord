#ifndef CLIENTSTATE_HPP_
#define CLIENTSTATE_HPP_

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class ClientState {
public:
  ClientState();

  void resetSession();
  void resetGuildChannels();
  void resetDmChannels();
  void resetGatewayPendingUpdates();

  QVariantMap dmChannelsById;
  QVariantMap dmChannelIdsByRecipientId;
  QVariantMap allDmChannelIndexById;
  QVariantMap visibleDmChannelIndexById;
  QStringList orderedGuildIds;
  QVariantList guildFolders;
  QVariantMap dmPresenceByUserId;
  QString lastGuildId;
  QString lastDmChannelId;
  QString selectedGuildId;
  QVariantList guilds;
  QVariantList allDmChannels;
  QVariantList dmChannels;
  QVariantList allGuildChannels;
  QVariantList visibleGuildChannels;
  QVariantMap pendingMentionCountsByGuildId;
  QVariantMap pendingMentionCountsByChannelId;
  QStringList pendingUnreadGuildIds;
  QStringList pendingUnreadChannelIds;
  QStringList pendingDmPresenceUserIds;
  int visibleDmChannelCount;
  int visibleGuildChannelCount;
  bool loadingGuilds;
  bool loadingDmChannels;
  bool loadingGuildChannels;
  bool guildsHasMore;
  bool dmChannelsHasMore;
  bool guildChannelsHasMore;
  bool gatewayUiUpdateQueued;
  bool pendingDmUiUpdate;
  bool guildsCacheSaveQueued;
  bool dmCacheSaveQueued;
  bool bootstrapCacheLoaded;
};

#endif /* CLIENTSTATE_HPP_ */
