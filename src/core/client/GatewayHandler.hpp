#ifndef GATEWAYHANDLER_HPP_
#define GATEWAYHANDLER_HPP_

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "../models/Models.hpp"

class DiscordClient;

class AppStore;
class QTimer;

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
                                 QVariantMap &pendingMentionCountsByChannelId,
                                 QStringList &pendingUnreadChannelIds,
                                 bool &pendingDmUiUpdate,
                                 bool &gatewayUiUpdateQueued);

  bool gatewayMessageMentionsCurrentUser(const QVariantMap &payload) const;

private Q_SLOTS:
  void flushMessageQueue();

private:
  bool shouldApplyChatEvent(const QString &channelId) const;

  DiscordClient *m_client;

  AppStore *m_store;
  QList<DiscordMessage> m_messageQueue;
  QTimer *m_batchTimer;
};

#endif /* GATEWAYHANDLER_HPP_ */
