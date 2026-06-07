#ifndef MainPageController_HPP_
#define MainPageController_HPP_

#include <QObject>
#include <QString>

class DiscordClient;

class MainPageController : public QObject {
  Q_OBJECT

public:
  explicit MainPageController(DiscordClient *client, QObject *parent = 0);

  Q_INVOKABLE void loadMainData();
  Q_INVOKABLE void loadGuildIcon(const QString &guildId);
  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectHome();
  Q_INVOKABLE void selectGuild(const QString &guildId);

private:
  DiscordClient *m_client;
};

#endif /* MainPageController_HPP_ */
