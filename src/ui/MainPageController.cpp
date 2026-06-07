#include "MainPageController.hpp"

#include "../core/Client.hpp"

MainPageController::MainPageController(DiscordClient *client, QObject *parent)
    : QObject(parent), m_client(client) {}

void MainPageController::loadMainData() {
  if (m_client) {
    m_client->loadMainData();
  }
}

void MainPageController::loadGuildIcon(const QString &guildId) {
  if (m_client) {
    m_client->loadGuildIcon(guildId);
  }
}

void MainPageController::loadMoreDmChannels() {
  if (m_client) {
    m_client->loadMoreDmChannels();
  }
}

void MainPageController::loadMoreGuildChannels() {
  if (m_client) {
    m_client->loadMoreGuildChannels();
  }
}

void MainPageController::selectHome() {
  if (m_client) {
    m_client->selectHome();
  }
}

void MainPageController::selectGuild(const QString &guildId) {
  if (m_client) {
    m_client->selectGuild(guildId);
  }
}
