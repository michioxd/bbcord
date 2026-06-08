#include "NetworkWorker.hpp"

#include "DiscordUtils.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QThread>

namespace {
bool isInObjectThread(const QObject *object) {
  return object != 0 && object->thread() == QThread::currentThread();
}

bool applyGuildOrder(QVariantList &guilds, const QStringList &orderedGuildIds) {
  if (orderedGuildIds.isEmpty() || guilds.isEmpty()) {
    return false;
  }

  QVariantList ordered;
  QStringList usedIds;
  for (int i = 0; i < orderedGuildIds.size(); ++i) {
    QString guildId = orderedGuildIds.at(i).trimmed();
    if (guildId.isEmpty() || usedIds.contains(guildId)) {
      continue;
    }

    for (int j = 0; j < guilds.size(); ++j) {
      QVariantMap guild = guilds.at(j).toMap();
      if (guild.value("id").toString() == guildId) {
        ordered.append(guild);
        usedIds.append(guildId);
        break;
      }
    }
  }

  if (ordered.isEmpty()) {
    return false;
  }

  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    if (!usedIds.contains(guild.value("id").toString())) {
      ordered.append(guild);
    }
  }

  guilds = ordered;
  return true;
}

bool applyGuildOrderFromGatewayPayload(QVariantList &guilds,
                                       QStringList &orderedGuildIds,
                                       const QVariantMap &payload) {
  QStringList newOrderedGuildIds;

  DiscordUtils::appendGuildIdsFromUserSettingsProto(
      &newOrderedGuildIds, payload.value("user_settings_proto").toString());

  QVariantMap settings = payload.value("settings").toMap();
  DiscordUtils::appendGuildIdsFromUserSettingsProto(
      &newOrderedGuildIds, settings.value("proto").toString());

  QVariantList folders = payload.value("guild_folders").toList();
  if (folders.isEmpty()) {
    QVariantMap userSettings = payload.value("user_settings").toMap();
    folders = userSettings.value("guild_folders").toList();
  }

  for (int i = 0; i < folders.size(); ++i) {
    QVariantMap folder = folders.at(i).toMap();
    QVariantList guildIds = folder.value("guild_ids").toList();
    for (int j = 0; j < guildIds.size(); ++j) {
      DiscordUtils::appendUniqueGuildId(&newOrderedGuildIds,
                                        guildIds.at(j).toString());
    }
  }

  if (newOrderedGuildIds.isEmpty()) {
    return false;
  }

  orderedGuildIds = newOrderedGuildIds;
  qDebug() << "[discord-worker] applied guild order from gateway"
           << orderedGuildIds.size();

  return applyGuildOrder(guilds, orderedGuildIds);
}

QString presenceStatusForRecipients(const QVariantList &userIds,
                                    const QVariantMap &dmPresenceByUserId) {
  QString bestStatus = "offline";
  int bestRank = 0;
  for (int i = 0; i < userIds.size(); ++i) {
    QString userId = userIds.at(i).toString().trimmed();
    QString status = dmPresenceByUserId.value(userId).toString();
    int rank = 0;
    if (status == "online") {
      rank = 3;
    } else if (status == "dnd" || status == "busy") {
      rank = 2;
    } else if (status == "idle") {
      rank = 1;
    }

    if (rank > bestRank) {
      bestRank = rank;
      bestStatus = status;
    }
  }
  return bestStatus;
}

QString presenceColor(const QString &status) {
  if (status == "online") {
    return "#23A55A";
  }
  if (status == "idle") {
    return "#F0B232";
  }
  if (status == "dnd" || status == "busy") {
    return "#F23F43";
  }
  return "#80848E";
}

void applyPresenceToDmChannels(QVariantList *channels,
                               const QVariantMap &dmPresenceByUserId) {
  if (channels == 0) {
    return;
  }

  for (int i = 0; i < channels->size(); ++i) {
    QVariantMap channel = channels->at(i).toMap();
    QVariantList recipientIds = channel.value("recipientIds").toList();
    QString status =
        presenceStatusForRecipients(recipientIds, dmPresenceByUserId);
    channel["status"] = status;
    channel["statusColor"] = presenceColor(status);
    channels->replace(i, channel);
  }
}

void moveDmToTop(QVariantList *channels, const QString &channelId,
                 const QString &lastMessageId) {
  if (channels == 0 || channelId.isEmpty()) {
    return;
  }

  for (int i = 0; i < channels->size(); ++i) {
    QVariantMap item = channels->at(i).toMap();
    if (item.value("id").toString() == channelId) {
      if (!lastMessageId.isEmpty()) {
        item["lastMessageId"] = lastMessageId;
      }
      channels->removeAt(i);
      channels->prepend(item);
      return;
    }
  }
}
} // namespace

DiscordNetworkWorker::DiscordNetworkWorker(QObject *parent)
    : QObject(parent), m_loginClient(this), m_dataClient(this),
      m_chatClient(this), m_avatarClient(this), m_avatarClient2(this),
      m_guildIconClient(this), m_guildIconClient2(this), m_gateway(this) {
  connect(&m_loginClient, SIGNAL(loginSucceeded(QVariantMap)), this,
          SIGNAL(loginSucceeded(QVariantMap)));
  connect(&m_loginClient, SIGNAL(loginFailed(QString)), this,
          SIGNAL(loginFailed(QString)));

  connect(&m_dataClient, SIGNAL(guildsLoaded(QVariantList)), this,
          SIGNAL(guildsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(dmChannelsLoaded(QVariantList)), this,
          SIGNAL(dmChannelsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(guildChannelsLoaded(QString, QVariantList)),
          this, SIGNAL(guildChannelsLoaded(QString, QVariantList)));
  connect(&m_dataClient, SIGNAL(requestFailed(QString)), this,
          SIGNAL(requestFailed(QString)));

  connect(&m_chatClient,
          SIGNAL(channelMessagesLoaded(QString, QString, QVariantList)), this,
          SIGNAL(channelMessagesLoaded(QString, QString, QVariantList)));
  connect(&m_chatClient,
          SIGNAL(channelMessageSent(QString, QString, QVariantMap)), this,
          SIGNAL(channelMessageSent(QString, QString, QVariantMap)));
  connect(&m_chatClient, SIGNAL(channelMessageEdited(QString, QVariantMap)),
          this, SIGNAL(channelMessageEdited(QString, QVariantMap)));
  connect(&m_chatClient, SIGNAL(channelMessageDeleted(QString, QString)), this,
          SIGNAL(channelMessageDeleted(QString, QString)));
  connect(&m_chatClient,
          SIGNAL(chatRequestFailed(QString, QString, QString, QString)), this,
          SIGNAL(chatRequestFailed(QString, QString, QString, QString)));

  connect(&m_avatarClient, SIGNAL(avatarDownloaded(QString, QString)), this,
          SIGNAL(avatarDownloaded(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloaded(QString, QString)), this,
          SIGNAL(avatarDownloaded(QString, QString)));
  connect(&m_avatarClient, SIGNAL(avatarDownloadFailed(QString, QString)), this,
          SIGNAL(avatarDownloadFailed(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloadFailed(QString, QString)),
          this, SIGNAL(avatarDownloadFailed(QString, QString)));

  connect(&m_guildIconClient, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SIGNAL(guildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient2, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SIGNAL(guildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient, SIGNAL(guildIconDownloadFailed(QString, QString)),
          this, SIGNAL(guildIconDownloadFailed(QString, QString)));
  connect(&m_guildIconClient2,
          SIGNAL(guildIconDownloadFailed(QString, QString)), this,
          SIGNAL(guildIconDownloadFailed(QString, QString)));

  connect(&m_gateway, SIGNAL(dispatchReceived(QString, QVariantMap)), this,
          SLOT(onGatewayDispatchReceived(QString, QVariantMap)));
  connect(&m_gateway, SIGNAL(error(QString)), this,
          SIGNAL(gatewayError(QString)));
  connect(&m_gateway, SIGNAL(closed()), this, SIGNAL(gatewayClosed()));
  connect(&m_gateway, SIGNAL(ready(QString)), this,
          SIGNAL(gatewayReady(QString)));
}

DiscordNetworkWorker::~DiscordNetworkWorker() { cancelAll(); }

void DiscordNetworkWorker::loginWithToken(const QString &token) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "loginWithToken", Qt::QueuedConnection,
                              Q_ARG(QString, token));
    return;
  }

  m_loginClient.loginWithToken(token);
}

void DiscordNetworkWorker::fetchGuilds(const QString &token, int limit,
                                       const QString &afterId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "fetchGuilds", Qt::QueuedConnection,
                              Q_ARG(QString, token), Q_ARG(int, limit),
                              Q_ARG(QString, afterId));
    return;
  }

  m_dataClient.fetchGuilds(token, limit, afterId);
}

void DiscordNetworkWorker::fetchDmChannels(const QString &token, int limit,
                                           const QString &afterId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "fetchDmChannels", Qt::QueuedConnection,
                              Q_ARG(QString, token), Q_ARG(int, limit),
                              Q_ARG(QString, afterId));
    return;
  }

  m_dataClient.fetchDmChannels(token, limit, afterId);
}

void DiscordNetworkWorker::fetchGuildChannels(const QString &token,
                                              const QString &guildId, int limit,
                                              const QString &afterId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "fetchGuildChannels", Qt::QueuedConnection,
                              Q_ARG(QString, token), Q_ARG(QString, guildId),
                              Q_ARG(int, limit), Q_ARG(QString, afterId));
    return;
  }

  m_dataClient.fetchGuildChannels(token, guildId, limit, afterId);
}

void DiscordNetworkWorker::fetchChannelMessages(
    const QString &token, const QString &channelId, int limit,
    const QString &beforeMessageId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "fetchChannelMessages",
                              Qt::QueuedConnection, Q_ARG(QString, token),
                              Q_ARG(QString, channelId), Q_ARG(int, limit),
                              Q_ARG(QString, beforeMessageId));
    return;
  }

  m_chatClient.fetchChannelMessages(token, channelId, limit, beforeMessageId);
}

void DiscordNetworkWorker::sendChannelMessage(const QString &token,
                                              const QString &channelId,
                                              const QString &content,
                                              const QString &nonce,
                                              const QString &replyMessageId,
                                              const QString &attachmentPath) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "sendChannelMessage", Qt::QueuedConnection,
                              Q_ARG(QString, token), Q_ARG(QString, channelId),
                              Q_ARG(QString, content), Q_ARG(QString, nonce),
                              Q_ARG(QString, replyMessageId),
                              Q_ARG(QString, attachmentPath));
    return;
  }

  m_chatClient.sendChannelMessage(token, channelId, content, nonce,
                                  replyMessageId, attachmentPath);
}

void DiscordNetworkWorker::editChannelMessage(const QString &token,
                                              const QString &channelId,
                                              const QString &messageId,
                                              const QString &content) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "editChannelMessage", Qt::QueuedConnection,
                              Q_ARG(QString, token), Q_ARG(QString, channelId),
                              Q_ARG(QString, messageId),
                              Q_ARG(QString, content));
    return;
  }

  m_chatClient.editChannelMessage(token, channelId, messageId, content);
}

void DiscordNetworkWorker::deleteChannelMessage(const QString &token,
                                                const QString &channelId,
                                                const QString &messageId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "deleteChannelMessage",
                              Qt::QueuedConnection, Q_ARG(QString, token),
                              Q_ARG(QString, channelId),
                              Q_ARG(QString, messageId));
    return;
  }

  m_chatClient.deleteChannelMessage(token, channelId, messageId);
}

void DiscordNetworkWorker::downloadAvatar(const QString &userId,
                                          const QString &avatarHash,
                                          const QString &outputPath) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(
        this, "downloadAvatar", Qt::QueuedConnection, Q_ARG(QString, userId),
        Q_ARG(QString, avatarHash), Q_ARG(QString, outputPath));
    return;
  }

  m_avatarClient.downloadAvatar(userId, avatarHash, outputPath);
}

void DiscordNetworkWorker::downloadAvatar2(const QString &userId,
                                           const QString &avatarHash,
                                           const QString &outputPath) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(
        this, "downloadAvatar2", Qt::QueuedConnection, Q_ARG(QString, userId),
        Q_ARG(QString, avatarHash), Q_ARG(QString, outputPath));
    return;
  }

  m_avatarClient2.downloadAvatar(userId, avatarHash, outputPath);
}

void DiscordNetworkWorker::downloadGuildIcon(const QString &guildId,
                                             const QString &iconHash,
                                             const QString &outputPath) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "downloadGuildIcon", Qt::QueuedConnection,
                              Q_ARG(QString, guildId), Q_ARG(QString, iconHash),
                              Q_ARG(QString, outputPath));
    return;
  }

  m_guildIconClient.downloadGuildIcon(guildId, iconHash, outputPath);
}

void DiscordNetworkWorker::downloadGuildIcon2(const QString &guildId,
                                              const QString &iconHash,
                                              const QString &outputPath) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "downloadGuildIcon2", Qt::QueuedConnection,
                              Q_ARG(QString, guildId), Q_ARG(QString, iconHash),
                              Q_ARG(QString, outputPath));
    return;
  }

  m_guildIconClient2.downloadGuildIcon(guildId, iconHash, outputPath);
}

void DiscordNetworkWorker::connectGateway(const QString &token) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "connectGateway", Qt::QueuedConnection,
                              Q_ARG(QString, token));
    return;
  }

  m_gateway.connectToGateway(token);
}

void DiscordNetworkWorker::disconnectGateway() {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "disconnectGateway", Qt::QueuedConnection);
    return;
  }

  m_gateway.disconnectFromGateway();
}

void DiscordNetworkWorker::updateGatewayOrderingState(
    const QVariantList &guilds, const QVariantList &allDmChannels,
    const QVariantList &visibleDmChannels, const QStringList &orderedGuildIds,
    const QVariantMap &dmPresenceByUserId) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "updateGatewayOrderingState",
                              Qt::QueuedConnection, Q_ARG(QVariantList, guilds),
                              Q_ARG(QVariantList, allDmChannels),
                              Q_ARG(QVariantList, visibleDmChannels),
                              Q_ARG(QStringList, orderedGuildIds),
                              Q_ARG(QVariantMap, dmPresenceByUserId));
    return;
  }

  m_gatewayGuilds = guilds;
  m_gatewayAllDmChannels = allDmChannels;
  m_gatewayVisibleDmChannels = visibleDmChannels;
  m_gatewayOrderedGuildIds = orderedGuildIds;
  m_gatewayDmPresenceByUserId = dmPresenceByUserId;
}

void DiscordNetworkWorker::onGatewayDispatchReceived(
    const QString &eventName, const QVariantMap &payload) {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "onGatewayDispatchReceived",
                              Qt::QueuedConnection, Q_ARG(QString, eventName),
                              Q_ARG(QVariantMap, payload));
    return;
  }

  emit gatewayDispatchReceived(eventName, payload);

  if (eventName == "MESSAGE_CREATE") {
    QString guildId = payload.value("guild_id").toString();
    QString channelId = payload.value("channel_id").toString().trimmed();
    if (guildId.isEmpty() && !channelId.isEmpty()) {
      QString lastMessageId = payload.value("id").toString().trimmed();
      moveDmToTop(&m_gatewayAllDmChannels, channelId, lastMessageId);
      moveDmToTop(&m_gatewayVisibleDmChannels, channelId, lastMessageId);
    }
    return;
  }

  if (eventName == "PRESENCE_UPDATE") {
    QString userId = payload.value("user").toMap().value("id").toString();
    QString status = payload.value("status").toString().trimmed();
    if (!userId.isEmpty()) {
      m_gatewayDmPresenceByUserId.insert(
          userId, status.isEmpty() ? QString("offline") : status);
    }
    return;
  }

  if (eventName == "READY") {
    QVariantList presences = payload.value("presences").toList();
    for (int i = 0; i < presences.size(); ++i) {
      QVariantMap presence = presences.at(i).toMap();
      QString userId = presence.value("user").toMap().value("id").toString();
      QString status = presence.value("status").toString().trimmed();
      if (!userId.isEmpty()) {
        m_gatewayDmPresenceByUserId.insert(
            userId, status.isEmpty() ? QString("offline") : status);
      }
    }
  }

  if (eventName != "GUILD_CREATE" && eventName != "GUILD_DELETE" &&
      eventName != "READY" && eventName != "USER_SETTINGS_PROTO_UPDATE") {
    return;
  }

  bool orderChanged = applyGuildOrderFromGatewayPayload(
      m_gatewayGuilds, m_gatewayOrderedGuildIds, payload);
  if (!orderChanged) {
    applyGuildOrder(m_gatewayGuilds, m_gatewayOrderedGuildIds);
  }

  applyPresenceToDmChannels(&m_gatewayAllDmChannels,
                            m_gatewayDmPresenceByUserId);
  applyPresenceToDmChannels(&m_gatewayVisibleDmChannels,
                            m_gatewayDmPresenceByUserId);
  DiscordUtils::stableSortItems(&m_gatewayAllDmChannels,
                                DiscordUtils::dmShouldMoveBefore);
  DiscordUtils::stableSortItems(&m_gatewayVisibleDmChannels,
                                DiscordUtils::dmShouldMoveBefore);

  emit guildsAndDmsReady(m_gatewayGuilds, m_gatewayAllDmChannels,
                         m_gatewayVisibleDmChannels, m_gatewayOrderedGuildIds,
                         m_gatewayDmPresenceByUserId);
}

void DiscordNetworkWorker::cancelAll() {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "cancelAll", Qt::QueuedConnection);
    return;
  }

  m_loginClient.cancel();
  m_dataClient.cancel();
  m_chatClient.cancel();
  m_avatarClient.cancel();
  m_avatarClient2.cancel();
  m_guildIconClient.cancel();
  m_guildIconClient2.cancel();
  m_gateway.disconnectFromGateway();
  emit cancelAllFinished();
}
