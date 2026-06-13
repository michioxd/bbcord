#ifndef MainPageController_HPP_
#define MainPageController_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>

class AppStore;
class DiscordClient;
class SettingsController;

class MainPageController : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      bb::cascades::DataModel *serverDataModel READ serverDataModel CONSTANT)

public:
  explicit MainPageController(DiscordClient *client, AppStore *store,
                              SettingsController *settings,
                              QObject *parent = 0);

  bb::cascades::DataModel *serverDataModel() const;

  Q_INVOKABLE void loadMainData();
  Q_INVOKABLE void loadGuildIcon(const QString &guildId);
  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectHome();
  Q_INVOKABLE void selectGuild(const QString &guildId);
  Q_INVOKABLE void toggleGuildFolder(const QString &folderId);

private Q_SLOTS:
  void onGuildsChanged();
  void onGuildFoldersChanged();
  void onGuildsReordered();
  void onGuildIconChanged(const QString &guildId, const QString &iconSource);
  void onSelectionChanged();

private:
  QVariantMap withSelectionState(const QVariantMap &item) const;
  QVariantList flattenedGuildItems() const;
  QVariantMap guildById(const QVariantMap &guildsById,
                        const QString &guildId) const;
  QString folderIdFor(const QVariantMap &folder) const;
  void refreshSelectionState();

  DiscordClient *m_client;
  AppStore *m_store;
  SettingsController *m_settings;
  bb::cascades::ArrayDataModel *m_serverDataModel;
};

#endif /* MainPageController_HPP_ */
