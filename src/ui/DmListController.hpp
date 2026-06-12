#ifndef DmListController_HPP_
#define DmListController_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>

#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/DataModel>

class AppStore;
class DiscordClient;

class DmListController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bb::cascades::DataModel *dmDataModel READ dmDataModel CONSTANT)

public:
  explicit DmListController(DiscordClient *client, AppStore *store,
                            QObject *parent = 0);

  bb::cascades::DataModel *dmDataModel() const;

  Q_INVOKABLE void loadMoreDmChannels();
  Q_INVOKABLE void loadDmAvatar(const QString &channelId);

private Q_SLOTS:
  void onDmChannelsChanged();
  void onDmChannelsAppended(const QVariantList &appended);
  void onDmAvatarChanged(const QString &channelId, const QString &avatarSource);
  void onDmAvatar2Changed(const QString &channelId,
                          const QString &avatarSource);
  void onDmStatusChanged(const QString &channelId, const QString &status,
                         const QString &statusColor);
  void onDataLoadingChanged(bool dataLoading);

private:
  void appendLoadingRowIfNeeded();
  void removeLoadingRow();

  DiscordClient *m_client;
  AppStore *m_store;
  bb::cascades::ArrayDataModel *m_dmDataModel;
};

#endif /* DmListController_HPP_ */
