#include "NetworkWorker.hpp"

#include <QMetaObject>
#include <QThread>

namespace {
bool isInObjectThread(const QObject *object) {
  return object != 0 && object->thread() == QThread::currentThread();
}
} // namespace

DiscordNetworkWorker::DiscordNetworkWorker(QObject *parent)
    : QObject(parent), m_loginClient(this), m_dataClient(this),
      m_avatarClient(this), m_avatarClient2(this), m_guildIconClient(this),
      m_guildIconClient2(this), m_gateway(this) {
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
          SIGNAL(gatewayDispatchReceived(QString, QVariantMap)));
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

void DiscordNetworkWorker::cancelAll() {
  if (!isInObjectThread(this)) {
    QMetaObject::invokeMethod(this, "cancelAll", Qt::QueuedConnection);
    return;
  }

  m_loginClient.cancel();
  m_dataClient.cancel();
  m_avatarClient.cancel();
  m_avatarClient2.cancel();
  m_guildIconClient.cancel();
  m_guildIconClient2.cancel();
  m_gateway.disconnectFromGateway();
  emit cancelAllFinished();
}
