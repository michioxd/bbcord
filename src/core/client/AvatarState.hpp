#ifndef AVATARSTATE_HPP_
#define AVATARSTATE_HPP_

#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class AvatarState {
public:
  AvatarState();

  void reset();

  QQueue<QVariantMap> pendingAvatars;
  QQueue<QVariantMap> pendingGuildIcons;
  QVariantMap avatarCacheRequests;
  QVariantMap guildIconCacheRequests;
  QVariantMap avatarSourcesByUserId;
  QString loadingAvatarUserId;
  QString loadingAvatarUserId2;
  QString loadingGuildIconId;
  QString loadingGuildIconId2;
  QStringList queuedAvatarUserIds;
  QStringList loadedAvatarUserIds;
  QStringList queuedGuildIconIds;
  QStringList loadedGuildIconIds;
};

#endif /* AVATARSTATE_HPP_ */
