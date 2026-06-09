#ifndef ServerListController_HPP_
#define ServerListController_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>

#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>

class AppStore;
class DiscordClient;

class ServerListController : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      bb::cascades::DataModel *channelDataModel READ channelDataModel CONSTANT)

public:
  explicit ServerListController(DiscordClient *client, AppStore *store,
                                QObject *parent = 0);

  bb::cascades::DataModel *channelDataModel() const;

  Q_INVOKABLE void loadGuildChannels(const QString &guildId);
  Q_INVOKABLE void loadMoreGuildChannels();
  Q_INVOKABLE void selectChannel(const QString &channelId);

private Q_SLOTS:
  void onGuildChannelsChanged();
  void onGuildChannelsAppended(const QVariantList &channels);
  void onDataLoadingChanged(bool dataLoading);

private:
  void removeChannelLoading();
  void updateChannelLoading();

  DiscordClient *m_client;
  AppStore *m_store;
  bb::cascades::ArrayDataModel *m_channelDataModel;
};

#endif /* ServerListController_HPP_ */
