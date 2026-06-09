#include "MainPageController.hpp"

#include "../core/AppStore.hpp"
#include "../core/Client.hpp"

MainPageController::MainPageController(DiscordClient *client, AppStore *store,
                                       QObject *parent)
    : QObject(parent), m_client(client), m_store(store),
      m_serverDataModel(new bb::cascades::ArrayDataModel(this)) {
  if (m_store) {
    connect(m_store, SIGNAL(guildsChanged()), this, SLOT(onGuildsChanged()));
    connect(m_store, SIGNAL(guildIconChanged(QString, QString)), this,
            SLOT(onGuildIconChanged(QString, QString)));
    onGuildsChanged();
  }
}

bb::cascades::DataModel *MainPageController::serverDataModel() const {
  return m_serverDataModel;
}

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

void MainPageController::onGuildsChanged() {
  m_serverDataModel->clear();

  QVariantMap home;
  home["type"] = "dm";
  home["name"] = "Home";
  home["id"] = "home";
  home["icon"] = "asset:///images/icons/first.png";
  home["mentionCount"] = 0;
  home["unread"] = false;
  m_serverDataModel->append(home);

  if (!m_store) {
    return;
  }

  QVariantList guilds = m_store->guilds();
  for (int i = 0; i < guilds.size(); ++i) {
    m_serverDataModel->append(guilds.at(i));
  }
}

void MainPageController::onGuildIconChanged(const QString &guildId,
                                            const QString &iconSource) {
  if (guildId.isEmpty() || iconSource.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_serverDataModel->size(); ++i) {
    QVariantMap item = m_serverDataModel->value(i).toMap();
    if (item.value("id").toString() == guildId) {
      item["icon"] = iconSource;
      m_serverDataModel->replace(i, item);
      return;
    }
  }
}
