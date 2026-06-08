#include "AvatarCacheWorker.hpp"

#include <QDir>
#include <QFileInfo>

AvatarCacheWorker::AvatarCacheWorker(QObject *parent) : QObject(parent) {}

void AvatarCacheWorker::checkAvatar(const QString &requestId,
                                    const QString &path) {
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    emit avatarCacheHit(requestId, path);
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  emit avatarCacheMiss(requestId, path);
}

void AvatarCacheWorker::checkGuildIcon(const QString &guildId,
                                       const QString &path) {
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    emit guildIconCacheHit(guildId, path);
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  emit guildIconCacheMiss(guildId, path);
}
