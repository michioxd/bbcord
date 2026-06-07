#include "ServerListController.hpp"

#include "../core/Client.hpp"

ServerListController::ServerListController(DiscordClient *client,
                                           QObject *parent)
    : QObject(parent), m_client(client) {}

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
