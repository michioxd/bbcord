#ifndef DmListController_HPP_
#define DmListController_HPP_

#include <QObject>
#include <QString>

class DiscordClient;

class DmListController : public QObject {
  Q_OBJECT

public:
  explicit DmListController(DiscordClient *client, QObject *parent = 0);

  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadDmAvatar(const QString &channelId);

private:
  DiscordClient *m_client;
};

#endif /* DmListController_HPP_ */
