#include "ServerListController.hpp"

#include "../core/AppStore.hpp"
#include "../core/Client.hpp"

ServerListController::ServerListController(DiscordClient *client,
                                           AppStore *store, QObject *parent)
    : QObject(parent), m_client(client), m_store(store),
      m_channelDataModel(new bb::cascades::ArrayDataModel(this)) {
  if (m_store) {
    connect(m_store, SIGNAL(guildChannelsChanged()), this,
            SLOT(onGuildChannelsChanged()));
    connect(m_store, SIGNAL(guildChannelsAppended(QVariantList)), this,
            SLOT(onGuildChannelsAppended(QVariantList)));
    connect(m_store, SIGNAL(dataLoadingChanged(bool)), this,
            SLOT(onDataLoadingChanged(bool)));
    onGuildChannelsChanged();
  }
}

bb::cascades::DataModel *ServerListController::channelDataModel() const {
  return m_channelDataModel;
}

void ServerListController::loadGuildChannels(const QString &guildId) {
  if (m_client) {
    m_client->loadGuildChannels(guildId);
  }
}

void ServerListController::loadMoreGuildChannels() {
  if (m_client) {
    m_client->loadMoreGuildChannels();
  }
}

void ServerListController::selectChannel(const QString &channelId) {
  if (m_client) {
    m_client->selectChannel(channelId);
  }
}

void ServerListController::onGuildChannelsChanged() {
  m_channelDataModel->clear();
  if (m_store) {
    QVariantList channels = m_store->guildChannels();
    for (int i = 0; i < channels.size(); ++i) {
      m_channelDataModel->append(channels.at(i));
    }
  }
  updateChannelLoading();
}

void ServerListController::onGuildChannelsAppended(
    const QVariantList &channels) {
  removeChannelLoading();
  for (int i = 0; i < channels.size(); ++i) {
    m_channelDataModel->append(channels.at(i));
  }
  updateChannelLoading();
}

void ServerListController::onDataLoadingChanged(bool dataLoading) {
  Q_UNUSED(dataLoading);
  updateChannelLoading();
}

void ServerListController::removeChannelLoading() {
  if (m_channelDataModel->size() == 0) {
    return;
  }

  int lastIndex = m_channelDataModel->size() - 1;
  QVariantMap last = m_channelDataModel->value(lastIndex).toMap();
  if (last.value("type").toString() == "loading") {
    m_channelDataModel->removeAt(lastIndex);
  }
}

void ServerListController::updateChannelLoading() {
  removeChannelLoading();
  if (!m_store || !m_store->dataLoading()) {
    return;
  }

  QVariantMap loading;
  loading["type"] = "loading";
  m_channelDataModel->append(loading);
}
