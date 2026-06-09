#include "DmListController.hpp"

#include "../core/AppStore.hpp"
#include "../core/Client.hpp"

DmListController::DmListController(DiscordClient *client, AppStore *store,
                                   QObject *parent)
    : QObject(parent), m_client(client), m_store(store),
      m_dmDataModel(new bb::cascades::ArrayDataModel(this)) {
  if (m_store) {
    connect(m_store, SIGNAL(dmChannelsChanged()), this,
            SLOT(onDmChannelsChanged()));
    connect(m_store, SIGNAL(dmChannelsAppended(QVariantList)), this,
            SLOT(onDmChannelsAppended(QVariantList)));
    connect(m_store, SIGNAL(dmAvatarChanged(QString, QString)), this,
            SLOT(onDmAvatarChanged(QString, QString)));
    connect(m_store, SIGNAL(dmAvatar2Changed(QString, QString)), this,
            SLOT(onDmAvatar2Changed(QString, QString)));
    connect(m_store, SIGNAL(dataLoadingChanged(bool)), this,
            SLOT(onDataLoadingChanged(bool)));
    onDmChannelsChanged();
  }
}

bb::cascades::DataModel *DmListController::dmDataModel() const {
  return m_dmDataModel;
}

void DmListController::loadMoreDmChannels() {
  if (m_client) {
    m_client->loadMoreDmChannels();
  }
}

void DmListController::loadDmAvatar(const QString &channelId) {
  if (m_client) {
    m_client->loadDmAvatar(channelId);
  }
}

void DmListController::onDmChannelsChanged() {
  m_dmDataModel->clear();
  if (m_store) {
    QVariantList channels = m_store->dmChannels();
    for (int i = 0; i < channels.size(); ++i) {
      m_dmDataModel->append(channels.at(i));
    }
  }
  appendLoadingRowIfNeeded();
}

void DmListController::onDmChannelsAppended(const QVariantList &appended) {
  removeLoadingRow();
  for (int i = 0; i < appended.size(); ++i) {
    m_dmDataModel->append(appended.at(i));
  }
  appendLoadingRowIfNeeded();
}

void DmListController::onDmAvatarChanged(const QString &channelId,
                                         const QString &avatarSource) {
  if (channelId.isEmpty() || avatarSource.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_dmDataModel->size(); ++i) {
    QVariantMap item = m_dmDataModel->value(i).toMap();
    if (item.value("id").toString() == channelId) {
      item["avatar"] = avatarSource;
      m_dmDataModel->replace(i, item);
      return;
    }
  }
}

void DmListController::onDmAvatar2Changed(const QString &channelId,
                                          const QString &avatarSource) {
  if (channelId.isEmpty() || avatarSource.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_dmDataModel->size(); ++i) {
    QVariantMap item = m_dmDataModel->value(i).toMap();
    if (item.value("id").toString() == channelId) {
      item["avatar2"] = avatarSource;
      m_dmDataModel->replace(i, item);
      return;
    }
  }
}

void DmListController::onDataLoadingChanged(bool dataLoading) {
  Q_UNUSED(dataLoading);
  removeLoadingRow();
  appendLoadingRowIfNeeded();
}

void DmListController::appendLoadingRowIfNeeded() {
  if (!m_store || !m_store->dataLoading()) {
    return;
  }

  QVariantMap loading;
  loading["type"] = "loading";
  m_dmDataModel->append(loading);
}

void DmListController::removeLoadingRow() {
  if (m_dmDataModel->size() == 0) {
    return;
  }

  int lastIndex = m_dmDataModel->size() - 1;
  QVariantMap last = m_dmDataModel->value(lastIndex).toMap();
  if (last.value("type").toString() == "loading") {
    m_dmDataModel->removeAt(lastIndex);
  }
}
