#ifndef ChannelMemberList_HPP_
#define ChannelMemberList_HPP_

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>

class AppStore;
class DiscordClient;

class ChannelMemberListController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentChannelId READ currentChannelId NOTIFY
                 currentChannelChanged)
  Q_PROPERTY(
      QString currentGuildId READ currentGuildId NOTIFY currentChannelChanged)
  Q_PROPERTY(QString currentChannelName READ currentChannelName NOTIFY
                 currentChannelChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(bb::cascades::DataModel *memberDataModel READ memberDataModel
                 NOTIFY memberDataModelChanged)

public:
  explicit ChannelMemberListController(DiscordClient *client, AppStore *store,
                                       QObject *parent = 0);

  QString currentChannelId() const;
  QString currentGuildId() const;
  QString currentChannelName() const;
  bool loading() const;
  bb::cascades::DataModel *memberDataModel() const;

  Q_INVOKABLE void openChannel(const QString &channelId, const QString &guildId,
                               const QString &channelName);
  Q_INVOKABLE void closeChannel();
  Q_INVOKABLE void loadMore();
  Q_INVOKABLE void loadMemberAvatar(const QString &userId,
                                    const QString &avatarHash);

Q_SIGNALS:
  void currentChannelChanged();
  void loadingChanged();
  void memberDataModelChanged();

private Q_SLOTS:
  void onGatewayDispatch(const QString &eventName, const QVariantMap &payload);
  void onGatewayClosed();
  void onLoadTimeout();
  void onChatAvatarChanged(const QString &userId, const QString &avatarSource);

private:
  QVariantMap memberItemFromPayload(const QVariantMap &member) const;
  QVariantMap roleItem(const QString &name, int count) const;
  void cacheGuildMetadata(const QVariantMap &guild);
  void applyGuildMetadata(const QVariantMap &guild);
  bool memberCanAccessCurrentChannel(const QVariantMap &member) const;
  QString topRoleIdForMember(const QVariantMap &member) const;
  QVariantMap presenceForUser(const QString &userId) const;
  bool isMemberOnline(const QString &userId) const;
  QString displayStatus(const QString &userId) const;
  QString statusColor(const QString &status) const;
  QString displayName(const QVariantMap &member, const QVariantMap &user) const;
  QString initialsForName(const QString &name) const;
  QString avatarColor(const QString &userId) const;
  QString activityText(const QVariantMap &presence) const;
  void requestRange(int start, int end);
  void rebuildModel();
  void appendMemberGroups(const QString &statusName,
                          const QHash<QString, QVariantList> &membersByRoleId,
                          const QVariantList &membersWithoutRole);
  void updateMemberRow(const QString &userId);
  void setLoading(bool loading);

  DiscordClient *m_client;
  AppStore *m_store;
  bb::cascades::ArrayDataModel *m_memberDataModel;
  QString m_currentChannelId;
  QString m_currentGuildId;
  QString m_currentChannelName;
  QHash<QString, QVariantMap> m_membersByUserId;
  QHash<QString, QVariantMap> m_lastListUpdateByChannelId;
  QHash<QString, QString> m_avatarSourcesByUserId;
  QHash<QString, int> m_modelIndexByUserId;
  QHash<QString, QVariantMap> m_guildMetadataById;
  QHash<QString, QVariantMap> m_rolesById;
  QHash<QString, QVariantMap> m_presencesByUserId;
  QSet<QString> m_channelMemberUserIds;
  QStringList m_roleIdsByOrder;
  QVariantList m_currentChannelOverwrites;
  QSet<QString> m_requestedAvatarUserIds;
  QString m_guildOwnerId;
  QString m_errorText;
  int m_expectedMemberCount;
  int m_loadedCount;
  int m_nextRangeStart;
  int m_lastRequestedStart;
  int m_lastRequestedEnd;
  bool m_loading;
  bool m_hasMore;
  bool m_usingChannelMemberList;
};

#endif /* ChannelMemberList_HPP_ */
