#ifndef GATEWAYHANDLER_HPP_
#define GATEWAYHANDLER_HPP_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class DiscordClient;

class AppStore;

class GatewayHandler : public QObject {

  Q_OBJECT
public:
  explicit GatewayHandler(DiscordClient *client, AppStore *store,
                          QObject *parent = 0);

  virtual ~GatewayHandler();

  void applyGatewayOrderingEvent(const QString &eventName,
                                 const QVariantMap &payload,
                                 QStringList &pendingUnreadGuildIds,
                                 QVariantMap &pendingMentionCountsByGuildId,
                                 QStringList &pendingUnreadChannelIds,
                                 bool &pendingDmUiUpdate,
                                 bool &gatewayUiUpdateQueued);

  bool gatewayMessageMentionsCurrentUser(const QVariantMap &payload) const;

private:
  bool shouldApplyChatEvent(const QString &channelId) const;

  DiscordClient *m_client;

  AppStore *m_store;
};

#endif /* GATEWAYHANDLER_HPP_ */
