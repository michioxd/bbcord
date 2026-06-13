#include "MainPageController.hpp"

#include "../core/AppStore.hpp"
#include "../core/Client.hpp"
#include "SettingsController.hpp"

#include <QStringList>

MainPageController::MainPageController(DiscordClient *client, AppStore *store,
                                       SettingsController *settings,
                                       QObject *parent)
    : QObject(parent), m_client(client), m_store(store), m_settings(settings),
      m_serverDataModel(new bb::cascades::ArrayDataModel(this)) {
  if (m_store) {
    connect(m_store, SIGNAL(guildsChanged()), this, SLOT(onGuildsChanged()));
    connect(m_store, SIGNAL(guildFoldersChanged()), this,
            SLOT(onGuildFoldersChanged()));
    connect(m_store, SIGNAL(guildsReordered()), this,
            SLOT(onGuildsReordered()));
    connect(m_store, SIGNAL(guildIconChanged(QString, QString)), this,
            SLOT(onGuildIconChanged(QString, QString)));
    connect(m_store, SIGNAL(selectionChanged()), this,
            SLOT(onSelectionChanged()));
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

void MainPageController::toggleGuildFolder(const QString &folderId) {
  if (!m_settings || folderId.trimmed().isEmpty()) {
    return;
  }

  bool expanded = !m_settings->guildFolderExpanded(folderId);
  m_settings->setGuildFolderExpanded(folderId, expanded);
  onGuildsChanged();
}

QVariantMap
MainPageController::withSelectionState(const QVariantMap &item) const {
  QVariantMap result = item;
  bool selected = false;
  if (m_store) {
    QString itemType = result.value("type").toString();
    QString itemId = result.value("id").toString();
    QString selectedGuildId = m_store->selectedGuildId();
    selected =
        (itemType == "dm" && itemId == "home" && selectedGuildId.isEmpty()) ||
        (itemType == "server" && !selectedGuildId.isEmpty() &&
         itemId == selectedGuildId);
  }
  result["active"] = selected;
  return result;
}

QVariantMap MainPageController::guildById(const QVariantMap &guildsById,
                                          const QString &guildId) const {
  return guildsById.value(guildId).toMap();
}

QString MainPageController::folderIdFor(const QVariantMap &folder) const {
  QString folderId = folder.value("id").toString().trimmed();
  if (!folderId.isEmpty()) {
    return folderId;
  }

  QVariantList guildIds = folder.value("guild_ids").toList();
  QStringList parts;
  for (int i = 0; i < guildIds.size(); ++i) {
    QString guildId = guildIds.at(i).toString().trimmed();
    if (!guildId.isEmpty()) {
      parts.append(guildId);
    }
  }
  return parts.join("_");
}

QVariantList MainPageController::flattenedGuildItems() const {
  QVariantList result;
  if (!m_store) {
    return result;
  }

  QVariantList guilds = m_store->guilds();
  QVariantMap guildsById;
  QStringList usedGuildIds;
  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    QString guildId = guild.value("id").toString();
    if (!guildId.isEmpty()) {
      guildsById.insert(guildId, guild);
    }
  }

  QVariantList folders = m_store->guildFolders();
  if (folders.isEmpty()) {
    return guilds;
  }

  for (int i = 0; i < folders.size(); ++i) {
    QVariantMap folder = folders.at(i).toMap();
    QVariantList folderGuildIds = folder.value("guild_ids").toList();
    QVariantList folderGuilds;
    for (int j = 0; j < folderGuildIds.size(); ++j) {
      QString guildId = folderGuildIds.at(j).toString();
      QVariantMap guild = guildById(guildsById, guildId);
      if (!guild.isEmpty()) {
        folderGuilds.append(guild);
      }
    }

    if (folderGuilds.size() <= 1) {
      for (int j = 0; j < folderGuilds.size(); ++j) {
        QVariantMap guild = folderGuilds.at(j).toMap();
        result.append(guild);
        usedGuildIds.append(guild.value("id").toString());
      }
      continue;
    }

    QString folderId = folderIdFor(folder);
    if (folderId.isEmpty()) {
      folderId = QString("folder_%1").arg(i);
    }
    bool expanded = m_settings && m_settings->guildFolderExpanded(folderId);
    QVariantMap folderItem;
    folderItem["type"] = "folder";
    folderItem["id"] = folderId;
    folderItem["name"] = folder.value("name").toString();
    if (folderItem.value("name").toString().isEmpty()) {
      folderItem["name"] = "Folder";
    }
    folderItem["expanded"] = expanded;
    folderItem["guilds"] = folderGuilds;
    folderItem["icon"] = QString();
    folderItem["initials"] = "F";
    folderItem["mentionCount"] = 0;
    folderItem["unread"] = false;
    int mentionCount = 0;
    bool unread = false;
    for (int j = 0; j < folderGuilds.size(); ++j) {
      QVariantMap guild = folderGuilds.at(j).toMap();
      mentionCount += guild.value("mentionCount").toInt();
      unread = unread || guild.value("unread").toBool();
    }
    folderItem["mentionCount"] = mentionCount;
    folderItem["unread"] = unread;
    result.append(folderItem);

    for (int j = 0; j < folderGuilds.size(); ++j) {
      QVariantMap guild = folderGuilds.at(j).toMap();
      usedGuildIds.append(guild.value("id").toString());
      if (expanded) {
        result.append(guild);
      }
    }
  }

  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    QString guildId = guild.value("id").toString();
    if (!usedGuildIds.contains(guildId)) {
      result.append(guild);
    }
  }

  return result;
}

void MainPageController::refreshSelectionState() {
  for (int i = 0; i < m_serverDataModel->size(); ++i) {
    QVariantMap item = m_serverDataModel->value(i).toMap();
    QVariantMap updatedItem = withSelectionState(item);
    if (item != updatedItem) {
      m_serverDataModel->replace(i, updatedItem);
    }
  }
}

void MainPageController::onGuildsChanged() {
  if (!m_store) {
    m_serverDataModel->clear();
    return;
  }

  QVariantList guilds = flattenedGuildItems();

  QVariantMap home;
  home["type"] = "dm";
  home["name"] = "Home";
  home["id"] = "home";
  home["icon"] = "asset:///images/icons/first.png";
  home["mentionCount"] = 0;
  home["unread"] = false;
  home = withSelectionState(home);

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
      QVariantMap newGuild = withSelectionState(guilds.at(i).toMap());
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
      QVariantMap newGuild = withSelectionState(guilds.at(i).toMap());
      if (currentGuild != newGuild) {
        m_serverDataModel->replace(i + 1, newGuild);
      }
    }
    return;
  }

  m_serverDataModel->clear();
  m_serverDataModel->append(home);
  for (int i = 0; i < guilds.size(); ++i) {
    m_serverDataModel->append(withSelectionState(guilds.at(i).toMap()));
  }
}

void MainPageController::onGuildFoldersChanged() { onGuildsChanged(); }

void MainPageController::onGuildsReordered() {
  if (!m_store) {
    m_serverDataModel->clear();
    return;
  }

  QVariantList guilds = flattenedGuildItems();
  if (m_serverDataModel->size() != guilds.size() + 1) {
    onGuildsChanged();
    return;
  }

  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap newGuild = withSelectionState(guilds.at(i).toMap());
    QString targetId = newGuild.value("id").toString();
    int targetIndex = i + 1;
    QVariantMap currentGuild = m_serverDataModel->value(targetIndex).toMap();
    if (currentGuild.value("id").toString() == targetId) {
      if (currentGuild != newGuild) {
        m_serverDataModel->replace(targetIndex, newGuild);
      }
      continue;
    }

    int sourceIndex = -1;
    for (int j = targetIndex + 1; j < m_serverDataModel->size(); ++j) {
      QVariantMap candidate = m_serverDataModel->value(j).toMap();
      if (candidate.value("id").toString() == targetId) {
        sourceIndex = j;
        break;
      }
    }

    if (sourceIndex < 0) {
      onGuildsChanged();
      return;
    }

    m_serverDataModel->removeAt(sourceIndex);
    m_serverDataModel->insert(targetIndex, newGuild);
  }
}

void MainPageController::onGuildIconChanged(const QString &guildId,
                                            const QString &iconSource) {
  if (guildId.isEmpty() || iconSource.isEmpty()) {
    return;
  }

  onGuildsChanged();
}

void MainPageController::onSelectionChanged() { refreshSelectionState(); }
