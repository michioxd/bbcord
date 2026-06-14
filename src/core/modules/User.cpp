#include "User.hpp"

#include "../Client.hpp"
#include "../client/AvatarManager.hpp"

QVariantMap DiscordClient::guildPresenceForUser(const QString &userId) const {
  QString safeUserId = userId.trimmed();
  if (safeUserId.isEmpty() || !m_dmPresenceByUserId.contains(safeUserId)) {
    return QVariantMap();
  }

  return m_dmPresenceByUserId.value(safeUserId).toMap();
}

QString DiscordClient::avatarSourceForUser(const QString &userId) const {
  return m_avatarSourcesByUserId.value(userId.trimmed()).toString();
}

void DiscordClient::loadUserAvatar(const QString &userId,
                                   const QString &avatarHash) {
  QString safeUserId = userId.trimmed();
  QString safeAvatarHash = avatarHash.trimmed();
  if (safeUserId.isEmpty() || safeAvatarHash.isEmpty() ||
      m_avatarManager == 0) {
    return;
  }

  m_avatarManager->queueDmAvatar(QString("chat"), safeUserId, safeAvatarHash,
                                 m_loadedAvatarUserIds, m_avatarCacheRequests,
                                 m_queuedAvatarUserIds, m_loadingAvatarUserId,
                                 m_loadingAvatarUserId2, m_pendingAvatars);
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
}
