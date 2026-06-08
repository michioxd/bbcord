#ifndef Client_HPP_
#define Client_HPP_

#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "models/Models.hpp"

class AppStore;
class DiscordNetworkWorker;
class QThread;

class DiscordClient : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
  explicit DiscordClient(QObject *parent = 0);
  explicit DiscordClient(AppStore *store, QObject *parent = 0);
  virtual ~DiscordClient();

  Q_INVOKABLE void login(const QString &token);
  Q_INVOKABLE void autoLogin();
  Q_INVOKABLE void logout();
  Q_INVOKABLE void loadMainData();
  Q_INVOKABLE void loadGuildIcon(const QString &guildId);
  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadDmAvatar(const QString &channelId);
  Q_INVOKABLE void loadGuildChannels(const QString &guildId);
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectHome();
  Q_INVOKABLE void selectGuild(const QString &guildId);
  Q_INVOKABLE void selectChannel(const QString &channelId);

  bool loggedIn() const;
  bool busy() const;
  QString statusText() const;

Q_SIGNALS:
  void loginSucceeded();
  void loginFailed(const QString &message);
  void loggedInChanged(bool loggedIn);
  void busyChanged(bool busy);
  void statusTextChanged(const QString &statusText);

private Q_SLOTS:
  void onRestLoginSucceeded(const QVariantMap &user);
  void onRestLoginFailed(const QString &message);
  void onGuildsLoaded(const QVariantList &guilds);
  void onDmChannelsLoaded(const QVariantList &channels);
  void onGuildChannelsLoaded(const QString &guildId,
                             const QVariantList &channels);
  void onDataRequestFailed(const QString &message);
  void onAvatarDownloaded(const QString &userId, const QString &localPath);
  void onAvatarDownloadFailed(const QString &userId, const QString &message);
  void onGuildIconDownloaded(const QString &guildId, const QString &localPath);
  void onGuildIconDownloadFailed(const QString &guildId,
                                 const QString &message);
  void onGatewayDispatch(const QString &eventName, const QVariantMap &payload);
  void flushGatewayUiUpdates();

private:
  void setLoggedIn(bool loggedIn);
  void setBusy(bool busy);
  void setStatusText(const QString &statusText);
  void saveToken();
  void clearSavedToken();
  void loadCurrentUserAvatar(const DiscordUser &user);
  void loadGuilds();
  void queueDmAvatar(const QString &channelId, const QString &userId,
                     const QString &avatarHash);
  void loadNextAvatar();
  void queueGuildIcon(const QString &guildId, const QString &iconHash);
  void loadNextGuildIcon();
  QString avatarCachePath(const DiscordUser &user) const;
  QString guildIconCachePath(const QString &guildId,
                             const QString &iconHash) const;
  QString avatarSourceForPath(const QString &path) const;
  QVariantMap guildToItem(const QVariantMap &guild) const;
  QVariantMap dmChannelToItem(const QVariantMap &channel) const;
  QVariantMap guildChannelToItem(const QVariantMap &channel) const;
  QString dmStatusForRecipients(const QVariantList &userIds) const;
  bool applyGuildOrder(const QStringList &orderedGuildIds);
  bool applyGuildOrderFromGatewayPayload(const QVariantMap &payload);
  void sortGuilds();
  int guildMentionCount(const QString &guildId) const;
  bool gatewayMessageMentionsCurrentUser(const QVariantMap &payload) const;
  void updateGuildMentionCount(const QString &guildId, int mentionCount);
  void updateGuildUnread(const QString &guildId, bool unread);
  void updateGuildChannelUnread(const QString &channelId, bool unread);
  void updateDmPresence(const QString &userId, const QString &status);
  bool applyPendingDmPresences();
  QVariantList
  sortedAccessibleGuildChannels(const QVariantList &channels) const;
  void moveDmToTop(const QString &channelId, const QString &lastMessageId);
  void applyGatewayOrderingEvent(const QString &eventName,
                                 const QVariantMap &payload);
  void appendVisibleGuildChannels();
  void updateDataLoading();
  void initializeNetworkWorker();
  void shutdownNetworkWorker();

  AppStore *m_store;
  QThread *m_networkThread;
  DiscordNetworkWorker *m_networkWorker;
  QQueue<QVariantMap> m_pendingAvatars;
  QQueue<QVariantMap> m_pendingGuildIcons;
  QString m_loadingAvatarUserId;
  QString m_loadingAvatarUserId2;
  QString m_loadingGuildIconId;
  QString m_loadingGuildIconId2;
  QStringList m_queuedAvatarUserIds;
  QStringList m_loadedAvatarUserIds;
  QStringList m_queuedGuildIconIds;
  QStringList m_loadedGuildIconIds;
  QStringList m_orderedGuildIds;
  QVariantMap m_dmPresenceByUserId;
  QString m_token;
  QString m_lastGuildId;
  QString m_lastDmChannelId;
  QString m_selectedGuildId;
  QVariantList m_guilds;
  QVariantList m_allDmChannels;
  QVariantList m_dmChannels;
  QVariantList m_allGuildChannels;
  QVariantList m_visibleGuildChannels;
  QVariantMap m_pendingMentionCountsByGuildId;
  QStringList m_pendingUnreadGuildIds;
  QStringList m_pendingUnreadChannelIds;
  QStringList m_pendingDmPresenceUserIds;
  int m_visibleDmChannelCount;
  int m_visibleGuildChannelCount;
  bool m_loadingGuilds;
  bool m_loadingDmChannels;
  bool m_loadingGuildChannels;
  bool m_guildsHasMore;
  bool m_dmChannelsHasMore;
  bool m_guildChannelsHasMore;
  bool m_loggedIn;
  bool m_busy;
  bool m_gatewayUiUpdateQueued;
  bool m_pendingDmUiUpdate;
  QString m_statusText;
};

#endif /* Client_HPP_ */
