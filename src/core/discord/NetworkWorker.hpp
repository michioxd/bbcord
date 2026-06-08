#ifndef NetworkWorker_HPP_
#define NetworkWorker_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "Gateway.hpp"
#include "RestClient.hpp"

class DiscordNetworkWorker : public QObject {
  Q_OBJECT

public:
  explicit DiscordNetworkWorker(QObject *parent = 0);
  virtual ~DiscordNetworkWorker();

public Q_SLOTS:
  void loginWithToken(const QString &token);
  void fetchGuilds(const QString &token, int limit, const QString &afterId);
  void fetchDmChannels(const QString &token, int limit, const QString &afterId);
  void fetchGuildChannels(const QString &token, const QString &guildId,
                          int limit, const QString &afterId);
  void downloadAvatar(const QString &userId, const QString &avatarHash,
                      const QString &outputPath);
  void downloadAvatar2(const QString &userId, const QString &avatarHash,
                       const QString &outputPath);
  void downloadGuildIcon(const QString &guildId, const QString &iconHash,
                         const QString &outputPath);
  void downloadGuildIcon2(const QString &guildId, const QString &iconHash,
                          const QString &outputPath);
  void connectGateway(const QString &token);
  void disconnectGateway();
  void cancelAll();

Q_SIGNALS:
  void loginSucceeded(const QVariantMap &user);
  void loginFailed(const QString &message);
  void guildsLoaded(const QVariantList &guilds);
  void dmChannelsLoaded(const QVariantList &channels);
  void guildChannelsLoaded(const QString &guildId,
                           const QVariantList &channels);
  void requestFailed(const QString &message);
  void avatarDownloaded(const QString &userId, const QString &localPath);
  void avatarDownloadFailed(const QString &userId, const QString &message);
  void guildIconDownloaded(const QString &guildId, const QString &localPath);
  void guildIconDownloadFailed(const QString &guildId, const QString &message);
  void gatewayDispatchReceived(const QString &eventName,
                               const QVariantMap &payload);
  void gatewayError(const QString &message);
  void gatewayClosed();
  void gatewayReady(const QString &sessionId);
  void cancelAllFinished();

private:
  DiscordRestClient m_loginClient;
  DiscordRestClient m_dataClient;
  DiscordRestClient m_avatarClient;
  DiscordRestClient m_avatarClient2;
  DiscordRestClient m_guildIconClient;
  DiscordRestClient m_guildIconClient2;
  DiscordGateway m_gateway;
};

#endif /* NetworkWorker_HPP_ */
