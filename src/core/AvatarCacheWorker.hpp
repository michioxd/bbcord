#ifndef AvatarCacheWorker_HPP_
#define AvatarCacheWorker_HPP_

#include <QObject>
#include <QString>

class AvatarCacheWorker : public QObject {
  Q_OBJECT

public:
  explicit AvatarCacheWorker(QObject *parent = 0);

public Q_SLOTS:
  void checkAvatar(const QString &requestId, const QString &path);
  void checkGuildIcon(const QString &guildId, const QString &path);

Q_SIGNALS:
  void avatarCacheHit(const QString &requestId, const QString &path);
  void avatarCacheMiss(const QString &requestId, const QString &path);
  void guildIconCacheHit(const QString &guildId, const QString &path);
  void guildIconCacheMiss(const QString &guildId, const QString &path);
};

#endif /* AvatarCacheWorker_HPP_ */
