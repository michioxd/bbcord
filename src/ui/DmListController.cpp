#include "DmListController.hpp"

#include "../core/Client.hpp"

DmListController::DmListController(DiscordClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

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
