#include "DM.hpp"

#include "../AppStore.hpp"
#include "../Client.hpp"
#include "../client/AvatarManager.hpp"
#include "../client/ItemMapper.hpp"
#include "../discord/DiscordUtils.hpp"
#include "../discord/NetworkWorker.hpp"
#include "FeatureConstants.hpp"

#include <QMetaObject>
#include <QSet>

void DiscordClient::loadMoreDmChannels() {
  if (m_loadingGuilds || m_loadingDmChannels || !m_dmChannelsHasMore ||
      m_token.trimmed().isEmpty()) {
    return;
  }

  if (m_visibleDmChannelCount < m_allDmChannels.size()) {
    int nextCount = m_visibleDmChannelCount + kPageSize;
    if (nextCount > m_allDmChannels.size()) {
      nextCount = m_allDmChannels.size();
    }

    QVariantList appendedChannels;
    for (int i = m_visibleDmChannelCount; i < nextCount; ++i) {
      QVariantMap channel = m_allDmChannels.at(i).toMap();
      m_dmChannels.append(channel);
      appendedChannels.append(channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
        m_visibleDmChannelIndexById.insert(channelId, m_dmChannels.size() - 1);
        indexDmChannelRecipients(channel);
      }
      m_lastDmChannelId = channelId;
    }

    m_visibleDmChannelCount = nextCount;
    m_dmChannelsHasMore = m_visibleDmChannelCount < m_allDmChannels.size();
    if (m_store) {
      m_store->appendDmChannels(appendedChannels);
    }
    syncGatewayOrderingStateToWorker();
    return;
  }

  m_loadingDmChannels = true;
  updateDataLoading();
  setStatusText("Loading DMs...");
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "fetchDmChannels",
                              Qt::QueuedConnection, Q_ARG(QString, m_token),
                              Q_ARG(int, kPageSize),
                              Q_ARG(QString, m_lastDmChannelId));
  }
}

void DiscordClient::loadDmAvatar(const QString &channelId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  QVariantMap channel = m_dmChannelsById.value(safeChannelId).toMap();
  if (channel.isEmpty()) {
    return;
  }

  m_avatarManager->queueDmAvatar(
      safeChannelId, channel.value("avatarUserId").toString(),
      channel.value("avatarHash").toString(), m_loadedAvatarUserIds,
      m_avatarCacheRequests, m_queuedAvatarUserIds, m_loadingAvatarUserId,
      m_loadingAvatarUserId2, m_pendingAvatars);
  m_avatarManager->queueDmAvatar(
      safeChannelId, channel.value("avatarUserId2").toString(),
      channel.value("avatarHash2").toString(), m_loadedAvatarUserIds,
      m_avatarCacheRequests, m_queuedAvatarUserIds, m_loadingAvatarUserId,
      m_loadingAvatarUserId2, m_pendingAvatars);
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
}

void DiscordClient::selectHome() {
  if (m_store) {
    m_store->selectHome();
    syncGatewayMessageFilterStateToWorker();
  }
  scheduleDmChannelsCacheSave();
  if (m_dmChannels.isEmpty()) {
    loadMoreDmChannels();
  }
}

void DiscordClient::onDmChannelsLoaded(const QVariantList &channels) {
  m_loadingDmChannels = false;
  updateDataLoading();
  QVariantMap existingAvatarsByUserId;
  for (QVariantMap::const_iterator it = m_avatarSourcesByUserId.constBegin();
       it != m_avatarSourcesByUserId.constEnd(); ++it) {
    if (!it.key().isEmpty() && !it.value().toString().isEmpty()) {
      existingAvatarsByUserId.insert(it.key(), it.value());
    }
  }
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap existingChannel = m_allDmChannels.at(i).toMap();
    QString userId = existingChannel.value("avatarUserId").toString();
    QString avatar = existingChannel.value("avatar").toString();
    if (!userId.isEmpty() && !avatar.isEmpty()) {
      existingAvatarsByUserId.insert(userId, avatar);
    }
    QString userId2 = existingChannel.value("avatarUserId2").toString();
    QString avatar2 = existingChannel.value("avatar2").toString();
    if (!userId2.isEmpty() && !avatar2.isEmpty()) {
      existingAvatarsByUserId.insert(userId2, avatar2);
    }
  }

  bool changed = false;
  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = m_itemMapper->dmChannelToItem(
        channels.at(i).toMap(), existingAvatarsByUserId, m_dmPresenceByUserId);
    QString channelId = item.value("id").toString();
    if (!channelId.isEmpty()) {
      QString avatar =
          existingAvatarsByUserId.value(item.value("avatarUserId").toString())
              .toString();
      if (!avatar.isEmpty()) {
        item["avatar"] = avatar;
      }
      QString avatar2 =
          existingAvatarsByUserId.value(item.value("avatarUserId2").toString())
              .toString();
      if (!avatar2.isEmpty()) {
        item["avatar2"] = avatar2;
      }

      int existingIndex = m_allDmChannelIndexById.value(channelId, -1).toInt();
      if (existingIndex >= 0 && existingIndex < m_allDmChannels.size()) {
        QVariantMap existingChannel = m_allDmChannels.at(existingIndex).toMap();
        item["status"] = existingChannel.value("status");
        item["statusColor"] = existingChannel.value("statusColor");
        item["unread"] = existingChannel.value("unread");
        m_allDmChannels.replace(existingIndex, item);

        int visibleIndex =
            m_visibleDmChannelIndexById.value(channelId, -1).toInt();
        if (visibleIndex >= 0 && visibleIndex < m_dmChannels.size()) {
          m_dmChannels.replace(visibleIndex, item);
          m_dmChannelsById.insert(channelId, item);
          changed = true;
        }
      } else {
        m_allDmChannels.append(item);
        m_allDmChannelIndexById.insert(channelId, m_allDmChannels.size() - 1);
        indexDmChannelRecipients(item);
        changed = true;
      }
    }
  }

  if (changed) {
    sortDmChannels();
    rebuildDmChannelIndexes();
    if (m_store) {
      m_store->setDmChannels(m_dmChannels);
    }
    syncGatewayOrderingStateToWorker();
  }

  m_dmChannelsHasMore = !channels.isEmpty();
  if (m_dmChannelsHasMore) {
    loadMoreDmChannels();
  }
  scheduleDmChannelsCacheSave();
  setStatusText("Connected");
}

void DiscordClient::sortDmChannels() {
  DiscordUtils::stableSortItems(&m_allDmChannels,
                                DiscordUtils::dmShouldMoveBefore);
  DiscordUtils::stableSortItems(&m_dmChannels,
                                DiscordUtils::dmShouldMoveBefore);
}

void DiscordClient::indexDmChannelRecipients(const QVariantMap &channel) {
  QString channelId = channel.value("id").toString();
  if (channelId.isEmpty()) {
    return;
  }

  QVariantList recipientIds = channel.value("recipientIds").toList();
  for (int i = 0; i < recipientIds.size(); ++i) {
    QString userId = recipientIds.at(i).toString().trimmed();
    if (userId.isEmpty()) {
      continue;
    }

    QStringList channelIds =
        m_dmChannelIdsByRecipientId.value(userId).toStringList();
    if (!channelIds.contains(channelId)) {
      channelIds.append(channelId);
      m_dmChannelIdsByRecipientId.insert(userId, channelIds);
    }
  }
}

void DiscordClient::rebuildDmRecipientIndex() {
  m_dmChannelIdsByRecipientId.clear();
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    indexDmChannelRecipients(m_allDmChannels.at(i).toMap());
  }
}

void DiscordClient::rebuildDmChannelIndexes() {
  m_allDmChannelIndexById.clear();
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QString channelId = m_allDmChannels.at(i).toMap().value("id").toString();
    if (!channelId.isEmpty()) {
      m_allDmChannelIndexById.insert(channelId, i);
    }
  }

  m_dmChannelsById.clear();
  m_visibleDmChannelIndexById.clear();
  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap channel = m_dmChannels.at(i).toMap();
    QString channelId = channel.value("id").toString();
    if (!channelId.isEmpty()) {
      m_dmChannelsById.insert(channelId, channel);
      m_visibleDmChannelIndexById.insert(channelId, i);
    }
  }
}

void DiscordClient::updateDmPresence(const QString &userId,
                                     const QString &status) {
  QString safeUserId = userId.trimmed();
  QString safeStatus = status.trimmed();
  if (safeUserId.isEmpty()) {
    return;
  }

  if (safeStatus.isEmpty()) {
    safeStatus = "offline";
  }

  if (m_dmPresenceByUserId.contains(safeUserId) &&
      m_dmPresenceByUserId.value(safeUserId).toString() == safeStatus) {
    return;
  }

  m_dmPresenceByUserId.insert(safeUserId, safeStatus);
  if (!m_pendingDmPresenceUserIds.contains(safeUserId)) {
    m_pendingDmPresenceUserIds.append(safeUserId);
  }
}

bool DiscordClient::applyPendingDmPresences() {
  if (m_pendingDmPresenceUserIds.isEmpty()) {
    return false;
  }

  QStringList userIds = m_pendingDmPresenceUserIds;
  m_pendingDmPresenceUserIds.clear();
  QStringList affectedChannelIds;
  QSet<QString> affectedChannelIdSet;

  for (int userIndex = 0; userIndex < userIds.size(); ++userIndex) {
    QStringList channelIds =
        m_dmChannelIdsByRecipientId.value(userIds.at(userIndex)).toStringList();
    for (int channelIndex = 0; channelIndex < channelIds.size();
         ++channelIndex) {
      QString channelId = channelIds.at(channelIndex);
      if (!channelId.isEmpty() && !affectedChannelIdSet.contains(channelId)) {
        affectedChannelIdSet.insert(channelId);
        affectedChannelIds.append(channelId);
      }
    }
  }

  bool changed = false;
  for (int i = 0; i < affectedChannelIds.size(); ++i) {
    QString channelId = affectedChannelIds.at(i);
    int allIndex = m_allDmChannelIndexById.value(channelId, -1).toInt();
    if (allIndex < 0 || allIndex >= m_allDmChannels.size()) {
      continue;
    }

    QVariantMap channel = m_allDmChannels.at(allIndex).toMap();
    QVariantList recipientIds = channel.value("recipientIds").toList();

    QString status = "offline";
    int bestRank = 0;
    for (int j = 0; j < recipientIds.size(); ++j) {
      QString uid = recipientIds.at(j).toString().trimmed();
      QString s = m_dmPresenceByUserId.value(uid).toString();
      int rank = 0;
      if (s == "online")
        rank = 3;
      else if (s == "dnd" || s == "busy")
        rank = 2;
      else if (s == "idle")
        rank = 1;

      if (rank > bestRank) {
        bestRank = rank;
        status = s;
      }
    }

    QString statusColor = "#80848E";
    if (status == "online")
      statusColor = "#23A55A";
    else if (status == "idle")
      statusColor = "#F0B232";
    else if (status == "dnd" || status == "busy")
      statusColor = "#F23F43";

    if (channel.value("status").toString() == status &&
        channel.value("statusColor").toString() == statusColor) {
      continue;
    }

    channel["status"] = status;
    channel["statusColor"] = statusColor;
    m_allDmChannels.replace(allIndex, channel);

    int visibleIndex = m_visibleDmChannelIndexById.value(channelId, -1).toInt();
    if (visibleIndex >= 0 && visibleIndex < m_dmChannels.size()) {
      QVariantMap visibleChannel = m_dmChannels.at(visibleIndex).toMap();
      visibleChannel["status"] = status;
      visibleChannel["statusColor"] = statusColor;
      m_dmChannels.replace(visibleIndex, visibleChannel);
      m_dmChannelsById.insert(channelId, visibleChannel);
      changed = true;
    } else if (m_dmChannelsById.contains(channelId)) {
      QVariantMap visibleChannel = m_dmChannelsById.value(channelId).toMap();
      if (!visibleChannel.isEmpty()) {
        visibleChannel["status"] = status;
        visibleChannel["statusColor"] = statusColor;
        m_dmChannelsById.insert(channelId, visibleChannel);
      }
    }
  }

  return changed;
}

void DiscordClient::moveDmToTop(const QString &channelId,
                                const QString &lastMessageId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap item = m_allDmChannels.at(i).toMap();
    if (item.value("id").toString() == safeChannelId) {
      if (!lastMessageId.trimmed().isEmpty()) {
        item["lastMessageId"] = lastMessageId.trimmed();
      }
      m_allDmChannels.removeAt(i);
      m_allDmChannels.prepend(item);
      rebuildDmChannelIndexes();
      break;
    }
  }

  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap item = m_dmChannels.at(i).toMap();
    if (item.value("id").toString() == safeChannelId) {
      if (!lastMessageId.trimmed().isEmpty()) {
        item["lastMessageId"] = lastMessageId.trimmed();
      }
      m_dmChannels.removeAt(i);
      m_dmChannels.prepend(item);
      rebuildDmChannelIndexes();
      return;
    }
  }
}
