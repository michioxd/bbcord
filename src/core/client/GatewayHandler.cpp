#include "GatewayHandler.hpp"
#include "../AppStore.hpp"
#include "../Client.hpp"
#include "../discord/DiscordUtils.hpp"
#include "../models/Models.hpp"

#include <QMetaObject>
#include <QTimer>

GatewayHandler::GatewayHandler(DiscordClient *client, AppStore *store,
                               QObject *parent)
    : QObject(parent), m_client(client), m_store(store), m_batchTimer(0) {
  m_batchTimer = new QTimer(this);
  m_batchTimer->setSingleShot(true);
  m_batchTimer->setInterval(100);
  connect(m_batchTimer, SIGNAL(timeout()), this, SLOT(flushMessageQueue()));
}
GatewayHandler::~GatewayHandler() { flushMessageQueue(); }
void GatewayHandler::applyGatewayOrderingEvent(
    const QString &eventName, const QVariantMap &payload,
    QStringList &pendingUnreadGuildIds,
    QVariantMap &pendingMentionCountsByGuildId,
    QStringList &pendingUnreadChannelIds, bool &pendingDmUiUpdate,
    bool &gatewayUiUpdateQueued) {
  if (eventName == "MESSAGE_CREATE") {
    QString guildId = payload.value("guild_id").toString();
    QString channelId = payload.value("channel_id").toString();
    if (shouldApplyChatEvent(channelId)) {
      DiscordMessage message = DiscordMessage::fromVariantMap(payload);
      message.pending = false;
      message.failed = false;
      m_messageQueue.append(message);
      m_batchTimer->start();
    }
    if (guildId.isEmpty() && !channelId.isEmpty()) {
      QMetaObject::invokeMethod(m_client, "moveDmToTop", Qt::DirectConnection,
                                Q_ARG(QString, channelId),
                                Q_ARG(QString, payload.value("id").toString()));
      pendingDmUiUpdate = true;
    } else if (!guildId.isEmpty() &&
               payload.value("author").toMap().value("id").toString() !=
                   (m_store ? m_store->currentUserId() : QString())) {
      if (!pendingUnreadGuildIds.contains(guildId)) {
        pendingUnreadGuildIds.append(guildId);
      }
      if (payload.contains("mention_count")) {
        pendingMentionCountsByGuildId.insert(
            guildId, payload.value("mention_count").toInt());
      } else if (gatewayMessageMentionsCurrentUser(payload)) {
        int mentionCount = 0;
        if (pendingMentionCountsByGuildId.contains(guildId)) {
          mentionCount = pendingMentionCountsByGuildId.value(guildId).toInt();
        } else {
          QMetaObject::invokeMethod(
              m_client, "guildMentionCount", Qt::DirectConnection,
              Q_RETURN_ARG(int, mentionCount), Q_ARG(QString, guildId));
        }
        pendingMentionCountsByGuildId.insert(guildId, mentionCount + 1);
      }
      if (!channelId.isEmpty() &&
          !pendingUnreadChannelIds.contains(channelId)) {
        pendingUnreadChannelIds.append(channelId);
      }
    }
    if (!gatewayUiUpdateQueued) {
      gatewayUiUpdateQueued = true;
      QTimer::singleShot(250, m_client, SLOT(flushGatewayUiUpdates()));
    }
    return;
  }
  if (eventName == "MESSAGE_UPDATE") {
    QString channelId = payload.value("channel_id").toString();
    if (shouldApplyChatEvent(channelId)) {
      DiscordMessage message = DiscordMessage::fromVariantMap(payload);
      message.pending = false;
      message.failed = false;
      m_store->updateChatMessage(message);
    }
    return;
  }
  if (eventName == "MESSAGE_DELETE") {
    QString channelId = payload.value("channel_id").toString();
    QString messageId = payload.value("id").toString();
    if (shouldApplyChatEvent(channelId)) {
      m_store->deleteChatMessage(channelId, messageId);
    }
    return;
  }
  if (eventName == "PRESENCE_UPDATE") {
    QMetaObject::invokeMethod(
        m_client, "updateDmPresence", Qt::DirectConnection,
        Q_ARG(QString, payload.value("user").toMap().value("id").toString()),
        Q_ARG(QString, payload.value("status").toString()));
    if (!gatewayUiUpdateQueued) {
      gatewayUiUpdateQueued = true;
      QTimer::singleShot(250, m_client, SLOT(flushGatewayUiUpdates()));
    }
    return;
  }
}

void GatewayHandler::flushMessageQueue() {
  if (m_messageQueue.isEmpty()) {
    return;
  }

  if (m_store != 0) {
    m_store->addOrReplaceChatMessages(m_messageQueue);
  }
  m_messageQueue.clear();
}

bool GatewayHandler::shouldApplyChatEvent(const QString &channelId) const {
  if (m_store == 0) {
    return false;
  }

  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return false;
  }

  return m_store->isChatInitialLoaded(safeChannelId) ||
         m_store->selectedChannelId() == safeChannelId;
}

bool GatewayHandler::gatewayMessageMentionsCurrentUser(
    const QVariantMap &payload) const {

  QString currentUserId = m_store ? m_store->currentUserId() : QString();

  if (currentUserId.isEmpty()) {

    return false;
  }
  if (payload.value("mention_everyone").toBool()) {
    return true;
  }
  QVariantList mentions = payload.value("mentions").toList();
  for (int i = 0; i < mentions.size(); ++i) {
    if (mentions.at(i).toMap().value("id").toString() == currentUserId) {
      return true;
    }
  }
  return false;
}
