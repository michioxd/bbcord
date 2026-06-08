#include "AppStore.hpp"

namespace {
const char *kDefaultUserAvatar = "asset:///images/icon.png";
}

AppStore::AppStore(QObject *parent)
    : QObject(parent), m_loggedIn(false), m_busy(false), m_dataLoading(false),
      m_statusText("Disconnected"),
      m_currentUserAvatarSource(kDefaultUserAvatar) {}

bool AppStore::loggedIn() const { return m_loggedIn; }

bool AppStore::busy() const { return m_busy; }

bool AppStore::dataLoading() const { return m_dataLoading; }

QString AppStore::statusText() const { return m_statusText; }

QString AppStore::currentUserName() const {
  return m_currentUser.displayName();
}

QString AppStore::currentUserId() const { return m_currentUser.id; }

QString AppStore::currentUserTag() const {
  if (m_currentUser.username.isEmpty()) {
    return QString();
  }

  if (!m_currentUser.discriminator.isEmpty() &&
      m_currentUser.discriminator != "0") {
    return QString("%1#%2")
        .arg(m_currentUser.username)
        .arg(m_currentUser.discriminator);
  }

  return m_currentUser.username;
}

QString AppStore::currentUserAvatarSource() const {
  if (m_currentUserAvatarSource.isEmpty()) {
    return kDefaultUserAvatar;
  }

  return m_currentUserAvatarSource;
}

QVariantList AppStore::guilds() const { return m_guilds; }

QVariantList AppStore::dmChannels() const { return m_dmChannels; }

QVariantList AppStore::guildChannels() const { return m_guildChannels; }

QString AppStore::selectedGuildId() const { return m_selectedGuildId; }

QString AppStore::selectedChannelId() const { return m_selectedChannelId; }

void AppStore::selectHome() {
  if (m_selectedGuildId.isEmpty() && m_selectedChannelId.isEmpty()) {
    return;
  }

  m_selectedGuildId.clear();
  m_selectedChannelId.clear();
  emit selectionChanged();
}

void AppStore::selectGuild(const QString &guildId) {
  if (m_selectedGuildId == guildId && m_selectedChannelId.isEmpty()) {
    return;
  }

  m_selectedGuildId = guildId;
  m_selectedChannelId.clear();
  emit selectionChanged();
}

void AppStore::selectChannel(const QString &channelId) {
  if (m_selectedChannelId == channelId) {
    return;
  }

  m_selectedChannelId = channelId;
  emit selectionChanged();
}

void AppStore::clearSession() {
  setLoggedIn(false);
  setBusy(false);
  setDataLoading(false);
  setStatusText("Disconnected");
  setCurrentUser(DiscordUser());
  setGuilds(QVariantList());
  setDmChannels(QVariantList());
  setGuildChannels(QVariantList());
  selectHome();
}

void AppStore::setLoggedIn(bool loggedIn) {
  if (m_loggedIn == loggedIn) {
    return;
  }

  m_loggedIn = loggedIn;
  emit loggedInChanged(m_loggedIn);
}

void AppStore::setBusy(bool busy) {
  if (m_busy == busy) {
    return;
  }

  m_busy = busy;
  emit busyChanged(m_busy);
}

void AppStore::setDataLoading(bool dataLoading) {
  if (m_dataLoading == dataLoading) {
    return;
  }

  m_dataLoading = dataLoading;
  emit dataLoadingChanged(m_dataLoading);
}

void AppStore::setStatusText(const QString &statusText) {
  if (m_statusText == statusText) {
    return;
  }

  m_statusText = statusText;
  emit statusTextChanged(m_statusText);
}

void AppStore::setCurrentUser(const DiscordUser &user) {
  if (m_currentUser.id == user.id && m_currentUser.username == user.username &&
      m_currentUser.globalName == user.globalName &&
      m_currentUser.discriminator == user.discriminator &&
      m_currentUser.avatarHash == user.avatarHash) {
    return;
  }

  m_currentUser = user;
  m_currentUserAvatarSource = kDefaultUserAvatar;
  emit currentUserChanged();
}

void AppStore::setCurrentUserAvatarSource(const QString &avatarSource) {
  QString safeAvatarSource = avatarSource.trimmed();
  if (safeAvatarSource.isEmpty()) {
    safeAvatarSource = kDefaultUserAvatar;
  }

  if (m_currentUserAvatarSource == safeAvatarSource) {
    return;
  }

  m_currentUserAvatarSource = safeAvatarSource;
  emit currentUserChanged();
}

void AppStore::setGuilds(const QVariantList &guilds) {
  m_guilds = guilds;
  emit guildsChanged();
}

void AppStore::updateGuildIcon(const QString &guildId,
                               const QString &iconSource) {
  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == guildId) {
      guild["icon"] = iconSource;
      m_guilds.replace(i, guild);
      emit guildIconChanged(guildId, iconSource);
      return;
    }
  }
}

void AppStore::updateDmAvatar(const QString &channelId,
                              const QString &avatarSource) {
  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap channel = m_dmChannels.at(i).toMap();
    if (channel.value("id").toString() == channelId) {
      channel["avatar"] = avatarSource;
      m_dmChannels.replace(i, channel);
      emit dmAvatarChanged(channelId, avatarSource);
      return;
    }
  }
}

void AppStore::updateDmAvatar2(const QString &channelId,
                               const QString &avatarSource) {
  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap channel = m_dmChannels.at(i).toMap();
    if (channel.value("id").toString() == channelId) {
      channel["avatar2"] = avatarSource;
      m_dmChannels.replace(i, channel);
      emit dmAvatar2Changed(channelId, avatarSource);
      return;
    }
  }
}

void AppStore::setDmChannels(const QVariantList &dmChannels) {
  QVariantList appended;
  if (dmChannels.size() >= m_dmChannels.size()) {
    for (int i = m_dmChannels.size(); i < dmChannels.size(); ++i) {
      appended.append(dmChannels.at(i));
    }
  }

  m_dmChannels = dmChannels;
  if (!appended.isEmpty()) {
    emit dmChannelsAppended(appended);
  }
  emit dmChannelsChanged();
}

void AppStore::setGuildChannels(const QVariantList &channels) {
  QVariantList appended;
  if (channels.size() >= m_guildChannels.size()) {
    for (int i = m_guildChannels.size(); i < channels.size(); ++i) {
      appended.append(channels.at(i));
    }
  }

  m_guildChannels = channels;
  if (!appended.isEmpty()) {
    emit guildChannelsAppended(appended);
  }
  emit guildChannelsChanged();
}
