#ifndef ServerListController_HPP_
#define ServerListController_HPP_

#include <QObject>
#include <QString>

class DiscordClient;

class ServerListController : public QObject {
  Q_OBJECT

public:
  explicit ServerListController(DiscordClient *client, QObject *parent = 0);

  Q_INVOKABLE void loadGuildChannels(const QString &guildId);
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectChannel(const QString &channelId);

private:
  DiscordClient *m_client;
};

#endif /* ServerListController_HPP_ */
