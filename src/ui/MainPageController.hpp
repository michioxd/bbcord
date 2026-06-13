#ifndef MainPageController_HPP_
#define MainPageController_HPP_

#include <QObject>
#include <QString>

#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>

class AppStore;
class DiscordClient;

class MainPageController : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      bb::cascades::DataModel *serverDataModel READ serverDataModel CONSTANT)

public:
  explicit MainPageController(DiscordClient *client, AppStore *store,
                              QObject *parent = 0);

  bb::cascades::DataModel *serverDataModel() const;

  Q_INVOKABLE void loadMainData();
  Q_INVOKABLE void loadGuildIcon(const QString &guildId);
  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectHome();
  Q_INVOKABLE void selectGuild(const QString &guildId);

private Q_SLOTS:
  void onGuildsChanged();
  void onGuildsReordered();
  void onGuildIconChanged(const QString &guildId, const QString &iconSource);

private:
  DiscordClient *m_client;
  AppStore *m_store;
  bb::cascades::ArrayDataModel *m_serverDataModel;
};

#endif /* MainPageController_HPP_ */
