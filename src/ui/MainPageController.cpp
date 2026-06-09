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
  if (!m_store) {
    m_serverDataModel->clear();
    return;
  }

  QVariantList guilds = m_store->guilds();

  QVariantMap home;
  home["type"] = "dm";
  home["name"] = "Home";
  home["id"] = "home";
  home["icon"] = "asset:///images/icons/first.png";
  home["mentionCount"] = 0;
  home["unread"] = false;

  bool canUpdateInPlace = m_serverDataModel->size() == guilds.size() + 1;
  if (canUpdateInPlace) {
    QVariantMap currentHome = m_serverDataModel->value(0).toMap();
    if (currentHome.value("id").toString() != "home") {
      canUpdateInPlace = false;
    }
  }
  if (canUpdateInPlace) {
    for (int i = 0; i < guilds.size(); ++i) {
      QVariantMap currentGuild = m_serverDataModel->value(i + 1).toMap();
      QVariantMap newGuild = guilds.at(i).toMap();
      if (currentGuild.value("id").toString() !=
          newGuild.value("id").toString()) {
        canUpdateInPlace = false;
        break;
      }
    }
  }

  if (canUpdateInPlace) {
    QVariantMap currentHome = m_serverDataModel->value(0).toMap();
    if (currentHome != home) {
      m_serverDataModel->replace(0, home);
    }
    for (int i = 0; i < guilds.size(); ++i) {
      QVariantMap currentGuild = m_serverDataModel->value(i + 1).toMap();
      QVariantMap newGuild = guilds.at(i).toMap();
      if (currentGuild != newGuild) {
        m_serverDataModel->replace(i + 1, newGuild);
      }
    }
    return;
  }

  m_serverDataModel->clear();
  m_serverDataModel->append(home);
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
