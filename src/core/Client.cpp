#include "Client.hpp"

#include "AppStore.hpp"
#include "models/Models.hpp"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSettings>
#include <QStringList>

namespace {
const int kPageSize = 25;
const int kGuildPageSize = 100;
const int kProtoFieldGuildFolders = 14;
const int kProtoFieldGuildFolderItems = 1;
const int kProtoFieldGuildFolderGuildIds = 1;

QString firstLetter(const QString &text) {
  if (text.isEmpty()) {
    return "?";
  }

  return text.left(1).toUpper();
}

QString firstTwoWordLetters(const QString &text) {
  QStringList parts = text.split(' ', QString::SkipEmptyParts);
  QString initials;
  for (int i = 0; i < parts.size() && initials.size() < 2; ++i) {
    initials += parts.at(i).left(1).toUpper();
  }

  if (initials.size() < 2 && parts.size() == 1 && parts.first().size() > 1) {
    initials = parts.first().left(2).toUpper();
  }

  if (initials.isEmpty()) {
    return firstLetter(text);
  }

  return initials;
}

QString cleanSnowflake(const QVariant &value) {
  return value.toString().trimmed();
}

void appendUniqueGuildId(QStringList *guildIds, const QString &guildId) {
  QString cleanGuildId = guildId.trimmed();
  if (guildIds == 0 || cleanGuildId.isEmpty() ||
      guildIds->contains(cleanGuildId)) {
    return;
  }

  guildIds->append(cleanGuildId);
}

bool readProtoVarint(const QByteArray &bytes, int *offset, quint64 *value) {
  if (offset == 0 || value == 0 || *offset < 0 || *offset >= bytes.size()) {
    return false;
  }

  quint64 result = 0;
  int shift = 0;
  while (*offset < bytes.size() && shift < 64) {
    unsigned char ch = static_cast<unsigned char>(bytes.at(*offset));
    ++(*offset);
    result |= (static_cast<quint64>(ch & 0x7f) << shift);
    if ((ch & 0x80) == 0) {
      *value = result;
      return true;
    }
    shift += 7;
  }

  return false;
}

bool readProtoLengthDelimited(const QByteArray &bytes, int *offset,
                              QByteArray *value) {
  quint64 length = 0;
  if (offset == 0 || value == 0 || !readProtoVarint(bytes, offset, &length) ||
      length > static_cast<quint64>(bytes.size() - *offset)) {
    return false;
  }

  *value = bytes.mid(*offset, static_cast<int>(length));
  *offset += static_cast<int>(length);
  return true;
}

bool skipProtoField(const QByteArray &bytes, int *offset, int wireType) {
  if (offset == 0) {
    return false;
  }

  switch (wireType) {
  case 0: {
    quint64 ignored = 0;
    return readProtoVarint(bytes, offset, &ignored);
  }
  case 1:
    if (*offset + 8 > bytes.size()) {
      return false;
    }
    *offset += 8;
    return true;
  case 2: {
    QByteArray ignored;
    return readProtoLengthDelimited(bytes, offset, &ignored);
  }
  case 5:
    if (*offset + 4 > bytes.size()) {
      return false;
    }
    *offset += 4;
    return true;
  default:
    return false;
  }
}

void appendGuildIdsFromProtoBytes(QStringList *orderedGuildIds,
                                  const QByteArray &guildBytes) {
  for (int i = 0; i + 7 < guildBytes.size(); i += 8) {
    quint64 guildId = 0;
    for (int j = 0; j < 8; ++j) {
      guildId |= (static_cast<quint64>(
                      static_cast<unsigned char>(guildBytes.at(i + j)))
                  << (j * 8));
    }
    appendUniqueGuildId(orderedGuildIds, QString::number(guildId));
  }
}

void appendGuildIdsFromProtoFolderItem(QStringList *orderedGuildIds,
                                       const QByteArray &itemBytes) {
  int offset = 0;
  while (offset < itemBytes.size()) {
    quint64 tag = 0;
    if (!readProtoVarint(itemBytes, &offset, &tag)) {
      return;
    }

    int fieldNumber = static_cast<int>(tag >> 3);
    int wireType = static_cast<int>(tag & 0x07);
    if (fieldNumber == kProtoFieldGuildFolderGuildIds && wireType == 2) {
      QByteArray guildBytes;
      if (!readProtoLengthDelimited(itemBytes, &offset, &guildBytes)) {
        return;
      }
      appendGuildIdsFromProtoBytes(orderedGuildIds, guildBytes);
    } else if (!skipProtoField(itemBytes, &offset, wireType)) {
      return;
    }
  }
}

void appendGuildIdsFromUserSettingsProto(QStringList *orderedGuildIds,
                                         const QString &base64Proto) {
  QByteArray settings = QByteArray::fromBase64(base64Proto.toLatin1());
  if (orderedGuildIds == 0 || settings.isEmpty()) {
    return;
  }

  int offset = 0;
  while (offset < settings.size()) {
    quint64 tag = 0;
    if (!readProtoVarint(settings, &offset, &tag)) {
      return;
    }

    int fieldNumber = static_cast<int>(tag >> 3);
    int wireType = static_cast<int>(tag & 0x07);
    if (fieldNumber == kProtoFieldGuildFolders && wireType == 2) {
      QByteArray guildFolders;
      if (!readProtoLengthDelimited(settings, &offset, &guildFolders)) {
        return;
      }

      int foldersOffset = 0;
      while (foldersOffset < guildFolders.size()) {
        quint64 folderTag = 0;
        if (!readProtoVarint(guildFolders, &foldersOffset, &folderTag)) {
          return;
        }

        int folderFieldNumber = static_cast<int>(folderTag >> 3);
        int folderWireType = static_cast<int>(folderTag & 0x07);
        if (folderFieldNumber == kProtoFieldGuildFolderItems &&
            folderWireType == 2) {
          QByteArray itemBytes;
          if (!readProtoLengthDelimited(guildFolders, &foldersOffset,
                                        &itemBytes)) {
            return;
          }
          appendGuildIdsFromProtoFolderItem(orderedGuildIds, itemBytes);
        } else if (!skipProtoField(guildFolders, &foldersOffset,
                                   folderWireType)) {
          return;
        }
      }
    } else if (!skipProtoField(settings, &offset, wireType)) {
      return;
    }
  }
}

bool snowflakeNewerThan(const QString &left, const QString &right) {
  if (left.isEmpty()) {
    return false;
  }

  if (right.isEmpty()) {
    return true;
  }

  if (left.size() != right.size()) {
    return left.size() > right.size();
  }

  return left > right;
}

bool positionShouldMoveBefore(const QVariantMap &left,
                              const QVariantMap &right) {
  if (!left.contains("position") || !right.contains("position")) {
    return false;
  }

  int leftPosition = left.value("position").toInt();
  int rightPosition = right.value("position").toInt();
  return leftPosition < rightPosition;
}

bool dmShouldMoveBefore(const QVariantMap &left, const QVariantMap &right) {
  return snowflakeNewerThan(cleanSnowflake(left.value("lastMessageId")),
                            cleanSnowflake(right.value("lastMessageId")));
}

void stableSortItems(QVariantList *items,
                     bool (*shouldMoveBefore)(const QVariantMap &left,
                                              const QVariantMap &right)) {
  if (items == 0 || items->size() < 2 || shouldMoveBefore == 0) {
    return;
  }

  for (int i = 1; i < items->size(); ++i) {
    QVariant current = items->at(i);
    QVariantMap currentMap = current.toMap();
    int j = i - 1;
    while (j >= 0 && shouldMoveBefore(currentMap, items->at(j).toMap())) {
      items->replace(j + 1, items->at(j));
      --j;
    }
    items->replace(j + 1, current);
  }
}
} // namespace

DiscordClient::DiscordClient(QObject *parent)
    : QObject(parent), m_store(0), m_restClient(this), m_dataClient(this),
      m_avatarClient(this), m_avatarClient2(this), m_guildIconClient(this),
      m_guildIconClient2(this), m_gateway(this), m_visibleDmChannelCount(0),
      m_visibleGuildChannelCount(0), m_loadingGuilds(false),
      m_loadingDmChannels(false), m_loadingGuildChannels(false),
      m_guildsHasMore(true), m_dmChannelsHasMore(true),
      m_guildChannelsHasMore(false), m_loggedIn(false), m_busy(false),
      m_statusText("Disconnected") {
  connect(&m_restClient, SIGNAL(loginSucceeded(QVariantMap)), this,
          SLOT(onRestLoginSucceeded(QVariantMap)));
  connect(&m_restClient, SIGNAL(loginFailed(QString)), this,
          SLOT(onRestLoginFailed(QString)));
  connect(&m_dataClient, SIGNAL(guildsLoaded(QVariantList)), this,
          SLOT(onGuildsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(dmChannelsLoaded(QVariantList)), this,
          SLOT(onDmChannelsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(guildChannelsLoaded(QString, QVariantList)),
          this, SLOT(onGuildChannelsLoaded(QString, QVariantList)));
  connect(&m_dataClient, SIGNAL(requestFailed(QString)), this,
          SLOT(onDataRequestFailed(QString)));
  connect(&m_avatarClient, SIGNAL(avatarDownloaded(QString, QString)), this,
          SLOT(onAvatarDownloaded(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloaded(QString, QString)), this,
          SLOT(onAvatarDownloaded(QString, QString)));
  connect(&m_avatarClient, SIGNAL(avatarDownloadFailed(QString, QString)), this,
          SLOT(onAvatarDownloadFailed(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloadFailed(QString, QString)),
          this, SLOT(onAvatarDownloadFailed(QString, QString)));
  connect(&m_guildIconClient, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SLOT(onGuildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient2, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SLOT(onGuildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient, SIGNAL(guildIconDownloadFailed(QString, QString)),
          this, SLOT(onGuildIconDownloadFailed(QString, QString)));
  connect(&m_guildIconClient2,
          SIGNAL(guildIconDownloadFailed(QString, QString)), this,
          SLOT(onGuildIconDownloadFailed(QString, QString)));
  connect(&m_gateway, SIGNAL(dispatchReceived(QString, QVariantMap)), this,
          SLOT(onGatewayDispatch(QString, QVariantMap)));

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::DiscordClient(AppStore *store, QObject *parent)
    : QObject(parent), m_store(store), m_restClient(this), m_dataClient(this),
      m_avatarClient(this), m_avatarClient2(this), m_guildIconClient(this),
      m_guildIconClient2(this), m_gateway(this), m_visibleDmChannelCount(0),
      m_visibleGuildChannelCount(0), m_loadingGuilds(false),
      m_loadingDmChannels(false), m_loadingGuildChannels(false),
      m_guildsHasMore(true), m_dmChannelsHasMore(true),
      m_guildChannelsHasMore(false), m_loggedIn(false), m_busy(false),
      m_statusText("Disconnected") {
  connect(&m_restClient, SIGNAL(loginSucceeded(QVariantMap)), this,
          SLOT(onRestLoginSucceeded(QVariantMap)));
  connect(&m_restClient, SIGNAL(loginFailed(QString)), this,
          SLOT(onRestLoginFailed(QString)));
  connect(&m_dataClient, SIGNAL(guildsLoaded(QVariantList)), this,
          SLOT(onGuildsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(dmChannelsLoaded(QVariantList)), this,
          SLOT(onDmChannelsLoaded(QVariantList)));
  connect(&m_dataClient, SIGNAL(guildChannelsLoaded(QString, QVariantList)),
          this, SLOT(onGuildChannelsLoaded(QString, QVariantList)));
  connect(&m_dataClient, SIGNAL(requestFailed(QString)), this,
          SLOT(onDataRequestFailed(QString)));
  connect(&m_avatarClient, SIGNAL(avatarDownloaded(QString, QString)), this,
          SLOT(onAvatarDownloaded(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloaded(QString, QString)), this,
          SLOT(onAvatarDownloaded(QString, QString)));
  connect(&m_avatarClient, SIGNAL(avatarDownloadFailed(QString, QString)), this,
          SLOT(onAvatarDownloadFailed(QString, QString)));
  connect(&m_avatarClient2, SIGNAL(avatarDownloadFailed(QString, QString)),
          this, SLOT(onAvatarDownloadFailed(QString, QString)));
  connect(&m_guildIconClient, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SLOT(onGuildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient2, SIGNAL(guildIconDownloaded(QString, QString)),
          this, SLOT(onGuildIconDownloaded(QString, QString)));
  connect(&m_guildIconClient, SIGNAL(guildIconDownloadFailed(QString, QString)),
          this, SLOT(onGuildIconDownloadFailed(QString, QString)));
  connect(&m_guildIconClient2,
          SIGNAL(guildIconDownloadFailed(QString, QString)), this,
          SLOT(onGuildIconDownloadFailed(QString, QString)));
  connect(&m_gateway, SIGNAL(dispatchReceived(QString, QVariantMap)), this,
          SLOT(onGatewayDispatch(QString, QVariantMap)));

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::~DiscordClient() {
  m_restClient.cancel();
  m_dataClient.cancel();
  m_avatarClient.cancel();
  m_avatarClient2.cancel();
  m_guildIconClient.cancel();
  m_guildIconClient2.cancel();
  m_gateway.disconnectFromGateway();
}

void DiscordClient::login(const QString &token) {
  QString trimmedToken = token.trimmed();
  if (trimmedToken.isEmpty()) {
    setStatusText("Token is empty");
    emit loginFailed(m_statusText);
    return;
  }

  setLoggedIn(false);
  setBusy(true);
  setStatusText("Checking Discord token...");

  m_token = trimmedToken;
  m_restClient.loginWithToken(trimmedToken);
}

void DiscordClient::autoLogin() {
  if (m_loggedIn || m_busy || m_token.trimmed().isEmpty()) {
    return;
  }

  qDebug() << "[discord-client] auto login with saved token";
  login(m_token);
}

void DiscordClient::logout() {
  clearSavedToken();
  m_restClient.cancel();
  m_dataClient.cancel();
  m_avatarClient.cancel();
  m_avatarClient2.cancel();
  m_guildIconClient.cancel();
  m_guildIconClient2.cancel();
  m_gateway.disconnectFromGateway();
  m_pendingAvatars.clear();
  m_pendingGuildIcons.clear();
  m_loadingAvatarUserId.clear();
  m_loadingAvatarUserId2.clear();
  m_loadingGuildIconId.clear();
  m_loadingGuildIconId2.clear();
  m_queuedAvatarUserIds.clear();
  m_loadedAvatarUserIds.clear();
  m_queuedGuildIconIds.clear();
  m_loadedGuildIconIds.clear();
  m_orderedGuildIds.clear();
  m_guilds.clear();
  m_allDmChannels.clear();
  m_dmChannels.clear();
  m_allGuildChannels.clear();
  m_visibleGuildChannels.clear();
  m_lastGuildId.clear();
  m_lastDmChannelId.clear();
  m_selectedGuildId.clear();
  m_visibleDmChannelCount = 0;
  m_visibleGuildChannelCount = 0;
  m_loadingGuilds = false;
  m_loadingDmChannels = false;
  m_loadingGuildChannels = false;
  m_guildsHasMore = true;
  m_dmChannelsHasMore = true;
  m_guildChannelsHasMore = false;
  if (m_store) {
    m_store->clearSession();
  }
  setBusy(false);
  setLoggedIn(false);
  setStatusText("Disconnected");
}

bool DiscordClient::loggedIn() const { return m_loggedIn; }

bool DiscordClient::busy() const { return m_busy; }

QString DiscordClient::statusText() const { return m_statusText; }

void DiscordClient::loadMainData() {
  if (!m_loggedIn || m_token.trimmed().isEmpty()) {
    return;
  }

  if (m_guilds.isEmpty()) {
    loadGuilds();
  } else if (m_dmChannels.isEmpty()) {
    loadMoreDmChannels();
  }
}

void DiscordClient::loadGuilds() {
  if (m_loadingGuilds || m_loadingDmChannels || m_loadingGuildChannels ||
      !m_guildsHasMore || m_token.trimmed().isEmpty()) {
    return;
  }

  m_loadingGuilds = true;
  updateDataLoading();
  setStatusText("Loading servers...");
  m_dataClient.fetchGuilds(m_token, kGuildPageSize, m_lastGuildId);
}

void DiscordClient::loadGuildIcon(const QString &guildId) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      queueGuildIcon(safeGuildId, guild.value("iconHash").toString());
      loadNextGuildIcon();
      return;
    }
  }
}

void DiscordClient::loadMoreDmChannels() {
  if (m_loadingGuilds || m_loadingDmChannels || m_loadingGuildChannels ||
      !m_dmChannelsHasMore || m_token.trimmed().isEmpty()) {
    return;
  }

  if (m_visibleDmChannelCount < m_allDmChannels.size()) {
    int nextCount = m_visibleDmChannelCount + kPageSize;
    if (nextCount > m_allDmChannels.size()) {
      nextCount = m_allDmChannels.size();
    }

    for (int i = m_visibleDmChannelCount; i < nextCount; ++i) {
      m_dmChannels.append(m_allDmChannels.at(i));
      m_lastDmChannelId = m_allDmChannels.at(i).toMap().value("id").toString();
    }

    m_visibleDmChannelCount = nextCount;
    m_dmChannelsHasMore = m_visibleDmChannelCount < m_allDmChannels.size();
    if (m_store) {
      m_store->setDmChannels(m_dmChannels);
    }
    return;
  }

  m_loadingDmChannels = true;
  updateDataLoading();
  setStatusText("Loading DMs...");
  m_dataClient.fetchDmChannels(m_token, kPageSize, m_lastDmChannelId);
}

void DiscordClient::loadDmAvatar(const QString &channelId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap channel = m_allDmChannels.at(i).toMap();
    if (channel.value("id").toString() == safeChannelId) {
      queueDmAvatar(safeChannelId, channel.value("avatarUserId").toString(),
                    channel.value("avatarHash").toString());
      loadNextAvatar();
      return;
    }
  }
}

void DiscordClient::loadGuildChannels(const QString &guildId) {
  if (m_loadingGuilds || m_loadingDmChannels || m_loadingGuildChannels ||
      guildId.trimmed().isEmpty() || m_token.trimmed().isEmpty()) {
    return;
  }

  if (guildId.trimmed() == m_selectedGuildId && !m_allGuildChannels.isEmpty()) {
    return;
  }

  m_selectedGuildId = guildId.trimmed();
  m_allGuildChannels.clear();
  m_visibleGuildChannels.clear();
  m_visibleGuildChannelCount = 0;
  m_guildChannelsHasMore = false;
  if (m_store) {
    m_store->setGuildChannels(QVariantList());
  }

  m_loadingGuildChannels = true;
  updateDataLoading();
  setStatusText("Loading channels...");
  m_dataClient.fetchGuildChannels(m_token, m_selectedGuildId, kPageSize,
                                  QString());
}

void DiscordClient::loadMoreGuildChannels() {
  if (!m_guildChannelsHasMore) {
    return;
  }

  appendVisibleGuildChannels();
}

void DiscordClient::selectHome() {
  if (m_store) {
    m_store->selectHome();
  }
  loadMoreDmChannels();
}

void DiscordClient::selectGuild(const QString &guildId) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  bool sameGuild = safeGuildId == m_selectedGuildId;

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId &&
        guild.value("unreadCount").toInt() > 0) {
      guild["unreadCount"] = 0;
      m_guilds.replace(i, guild);
      if (m_store) {
        m_store->setGuilds(m_guilds);
      }
      break;
    }
  }

  if (m_store) {
    m_store->selectGuild(safeGuildId);
  }
  if (!sameGuild || m_allGuildChannels.isEmpty()) {
    loadGuildChannels(safeGuildId);
  }
}

void DiscordClient::selectChannel(const QString &channelId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  updateGuildChannelUnread(safeChannelId, false);
  if (m_store) {
    m_store->selectChannel(safeChannelId);
  }
}

void DiscordClient::onRestLoginSucceeded(const QVariantMap &user) {
  qDebug() << "[discord-client] REST login succeeded"
           << user.value("id").toString();

  if (m_store) {
    DiscordUser currentUser;
    currentUser.id = user.value("id").toString();
    currentUser.username = user.value("username").toString();
    currentUser.discriminator = user.value("discriminator").toString();
    currentUser.globalName = user.value("global_name").toString();
    currentUser.avatarHash = user.value("avatar").toString();
    currentUser.bot = user.value("bot").toBool();
    m_store->setCurrentUser(currentUser);
    loadCurrentUserAvatar(currentUser);
  }

  saveToken();
  setBusy(false);
  setLoggedIn(true);
  setStatusText("Connected");
  m_gateway.connectToGateway(m_token);
  emit loginSucceeded();
}

void DiscordClient::onRestLoginFailed(const QString &message) {
  qDebug() << "[discord-client] REST login failed" << message;
  setBusy(false);
  setLoggedIn(false);
  m_gateway.disconnectFromGateway();
  setStatusText(message);
  emit loginFailed(message);
}

void DiscordClient::onGuildsLoaded(const QVariantList &guilds) {
  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    QVariantMap item = guildToItem(guild);
    if (!item.value("id").toString().isEmpty()) {
      m_guilds.append(item);
      m_lastGuildId = item.value("id").toString();
    }
  }

  if (guilds.size() >= kGuildPageSize && !m_lastGuildId.isEmpty()) {
    m_guildsHasMore = true;
    setStatusText("Loading servers...");
    m_dataClient.fetchGuilds(m_token, kGuildPageSize, m_lastGuildId);
    return;
  }

  m_loadingGuilds = false;
  m_guildsHasMore = false;
  updateDataLoading();

  sortGuilds();

  if (m_store) {
    m_store->setGuilds(m_guilds);
  }
  setStatusText("Connected");
  if (m_dmChannels.isEmpty()) {
    loadMoreDmChannels();
  }
}

void DiscordClient::onDmChannelsLoaded(const QVariantList &channels) {
  m_loadingDmChannels = false;
  updateDataLoading();
  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = dmChannelToItem(channels.at(i).toMap());
    if (!item.value("id").toString().isEmpty()) {
      m_allDmChannels.append(item);
    }
  }
  stableSortItems(&m_allDmChannels, dmShouldMoveBefore);

  m_dmChannelsHasMore = !m_allDmChannels.isEmpty();
  loadMoreDmChannels();
  setStatusText("Connected");
}

void DiscordClient::onGuildChannelsLoaded(const QString &guildId,
                                          const QVariantList &channels) {
  if (guildId != m_selectedGuildId) {
    return;
  }

  m_loadingGuildChannels = false;
  updateDataLoading();
  m_allGuildChannels.clear();
  QVariantList rawChannels;
  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = guildChannelToItem(channels.at(i).toMap());
    if (!item.value("id").toString().isEmpty()) {
      rawChannels.append(item);
    }
  }
  m_allGuildChannels = sortedAccessibleGuildChannels(rawChannels);

  appendVisibleGuildChannels();
  setStatusText("Connected");
}

void DiscordClient::onDataRequestFailed(const QString &message) {
  m_loadingGuilds = false;
  m_loadingDmChannels = false;
  m_loadingGuildChannels = false;
  updateDataLoading();
  qDebug() << "[discord-client] data request failed" << message;
  setStatusText(message);
}

void DiscordClient::onAvatarDownloaded(const QString &userId,
                                       const QString &localPath) {
  QString source = avatarSourceForPath(localPath);
  if (m_store && userId == m_store->currentUserId()) {
    m_store->setCurrentUserAvatarSource(avatarSourceForPath(localPath));
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap channel = m_allDmChannels.at(i).toMap();
    if (channel.value("avatarUserId").toString() == userId) {
      channel["avatar"] = source;
      m_allDmChannels.replace(i, channel);
    }
  }

  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap channel = m_dmChannels.at(i).toMap();
    if (channel.value("avatarUserId").toString() == userId) {
      channel["avatar"] = source;
      m_dmChannels.replace(i, channel);
      if (m_store) {
        m_store->updateDmAvatar(channel.value("id").toString(), source);
      }
    }
  }

  if (!m_loadedAvatarUserIds.contains(userId)) {
    m_loadedAvatarUserIds.append(userId);
  }
  if (m_loadingAvatarUserId == userId) {
    m_loadingAvatarUserId.clear();
  }
  if (m_loadingAvatarUserId2 == userId) {
    m_loadingAvatarUserId2.clear();
  }
  loadNextAvatar();
}

void DiscordClient::onAvatarDownloadFailed(const QString &userId,
                                           const QString &message) {
  qDebug() << "[discord-client] avatar download failed" << userId << message;
  m_queuedAvatarUserIds.removeAll(userId);
  if (m_loadingAvatarUserId == userId) {
    m_loadingAvatarUserId.clear();
  }
  if (m_loadingAvatarUserId2 == userId) {
    m_loadingAvatarUserId2.clear();
  }
  loadNextAvatar();
}

void DiscordClient::onGuildIconDownloaded(const QString &guildId,
                                          const QString &localPath) {
  QString source = avatarSourceForPath(localPath);
  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == guildId) {
      guild["icon"] = source;
      m_guilds.replace(i, guild);
      if (m_store) {
        m_store->updateGuildIcon(guildId, source);
      }
      break;
    }
  }

  if (!m_loadedGuildIconIds.contains(guildId)) {
    m_loadedGuildIconIds.append(guildId);
  }
  if (m_loadingGuildIconId == guildId) {
    m_loadingGuildIconId.clear();
  }
  if (m_loadingGuildIconId2 == guildId) {
    m_loadingGuildIconId2.clear();
  }
  loadNextGuildIcon();
}

void DiscordClient::onGuildIconDownloadFailed(const QString &guildId,
                                              const QString &message) {
  qDebug() << "[discord-client] guild icon download failed" << guildId
           << message;
  m_queuedGuildIconIds.removeAll(guildId);
  if (m_loadingGuildIconId == guildId) {
    m_loadingGuildIconId.clear();
  }
  if (m_loadingGuildIconId2 == guildId) {
    m_loadingGuildIconId2.clear();
  }
  loadNextGuildIcon();
}

void DiscordClient::onGatewayDispatch(const QString &eventName,
                                      const QVariantMap &payload) {
  applyGatewayOrderingEvent(eventName, payload);
}

void DiscordClient::setLoggedIn(bool loggedIn) {
  if (m_loggedIn == loggedIn) {
    return;
  }

  m_loggedIn = loggedIn;
  if (m_store) {
    m_store->setLoggedIn(m_loggedIn);
  }
  emit loggedInChanged(m_loggedIn);
}

void DiscordClient::setBusy(bool busy) {
  if (m_busy == busy) {
    return;
  }

  m_busy = busy;
  if (m_store) {
    m_store->setBusy(m_busy);
  }
  emit busyChanged(m_busy);
}

void DiscordClient::setStatusText(const QString &statusText) {
  if (m_statusText == statusText) {
    return;
  }

  m_statusText = statusText;
  if (m_store) {
    m_store->setStatusText(m_statusText);
  }
  emit statusTextChanged(m_statusText);
}

void DiscordClient::saveToken() {
  if (m_token.trimmed().isEmpty()) {
    return;
  }

  QSettings settings;
  settings.setValue("auth/token", m_token.trimmed());
  settings.sync();
}

void DiscordClient::clearSavedToken() {
  QSettings settings;
  settings.remove("auth/token");
  settings.sync();
  m_token.clear();
}

void DiscordClient::loadCurrentUserAvatar(const DiscordUser &user) {
  if (!m_store || user.id.isEmpty() || user.avatarHash.isEmpty()) {
    return;
  }

  QString path = avatarCachePath(user);
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    m_store->setCurrentUserAvatarSource(avatarSourceForPath(path));
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists() && !dir.mkpath(".")) {
    qDebug() << "[discord-client] could not create avatar cache"
             << dir.absolutePath();
    return;
  }

  if (m_loadingAvatarUserId.isEmpty()) {
    m_loadingAvatarUserId = user.id;
    m_avatarClient.downloadAvatar(user.id, user.avatarHash, path);
  } else if (m_loadingAvatarUserId2.isEmpty()) {
    m_loadingAvatarUserId2 = user.id;
    m_avatarClient2.downloadAvatar(user.id, user.avatarHash, path);
  } else {
    QVariantMap request;
    request["channelId"] = QString();
    request["userId"] = user.id;
    request["avatarHash"] = user.avatarHash;
    request["path"] = path;
    m_pendingAvatars.enqueue(request);
    m_queuedAvatarUserIds.append(user.id);
  }
}

void DiscordClient::queueDmAvatar(const QString &channelId,
                                  const QString &userId,
                                  const QString &avatarHash) {
  QString safeChannelId = channelId.trimmed();
  QString safeUserId = userId.trimmed();
  QString safeAvatarHash = avatarHash.trimmed();
  if (safeChannelId.isEmpty() || safeUserId.isEmpty() ||
      safeAvatarHash.isEmpty()) {
    return;
  }

  if (m_loadedAvatarUserIds.contains(safeUserId) ||
      m_queuedAvatarUserIds.contains(safeUserId) ||
      m_loadingAvatarUserId == safeUserId ||
      m_loadingAvatarUserId2 == safeUserId) {
    return;
  }

  DiscordUser user;
  user.id = safeUserId;
  user.avatarHash = safeAvatarHash;
  QString path = avatarCachePath(user);
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    onAvatarDownloaded(safeUserId, path);
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists() && !dir.mkpath(".")) {
    qDebug() << "[discord-client] could not create DM avatar cache"
             << dir.absolutePath();
    return;
  }

  QVariantMap request;
  request["channelId"] = safeChannelId;
  request["userId"] = safeUserId;
  request["avatarHash"] = safeAvatarHash;
  request["path"] = path;
  m_pendingAvatars.enqueue(request);
  m_queuedAvatarUserIds.append(safeUserId);
}

void DiscordClient::loadNextAvatar() {
  if (m_loadingAvatarUserId.isEmpty() && !m_pendingAvatars.isEmpty()) {
    QVariantMap request = m_pendingAvatars.dequeue();
    m_loadingAvatarUserId = request.value("userId").toString();
    m_queuedAvatarUserIds.removeAll(m_loadingAvatarUserId);
    m_avatarClient.downloadAvatar(m_loadingAvatarUserId,
                                  request.value("avatarHash").toString(),
                                  request.value("path").toString());
  }

  if (m_loadingAvatarUserId2.isEmpty() && !m_pendingAvatars.isEmpty()) {
    QVariantMap request = m_pendingAvatars.dequeue();
    m_loadingAvatarUserId2 = request.value("userId").toString();
    m_queuedAvatarUserIds.removeAll(m_loadingAvatarUserId2);
    m_avatarClient2.downloadAvatar(m_loadingAvatarUserId2,
                                   request.value("avatarHash").toString(),
                                   request.value("path").toString());
  }
}

void DiscordClient::queueGuildIcon(const QString &guildId,
                                   const QString &iconHash) {
  QString safeGuildId = guildId.trimmed();
  QString safeIconHash = iconHash.trimmed();
  if (safeGuildId.isEmpty() || safeIconHash.isEmpty()) {
    return;
  }

  if (m_loadedGuildIconIds.contains(safeGuildId) ||
      m_queuedGuildIconIds.contains(safeGuildId) ||
      m_loadingGuildIconId == safeGuildId ||
      m_loadingGuildIconId2 == safeGuildId) {
    return;
  }

  QString path = guildIconCachePath(safeGuildId, safeIconHash);
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    QString source = avatarSourceForPath(path);
    for (int i = 0; i < m_guilds.size(); ++i) {
      QVariantMap guild = m_guilds.at(i).toMap();
      if (guild.value("id").toString() == safeGuildId) {
        guild["icon"] = source;
        m_guilds.replace(i, guild);
        if (m_store) {
          m_store->updateGuildIcon(safeGuildId, source);
        }
        break;
      }
    }
    if (!m_loadedGuildIconIds.contains(safeGuildId)) {
      m_loadedGuildIconIds.append(safeGuildId);
    }
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists() && !dir.mkpath(".")) {
    qDebug() << "[discord-client] could not create guild icon cache"
             << dir.absolutePath();
    return;
  }

  QVariantMap request;
  request["guildId"] = safeGuildId;
  request["iconHash"] = safeIconHash;
  request["path"] = path;
  m_pendingGuildIcons.enqueue(request);
  m_queuedGuildIconIds.append(safeGuildId);
}

void DiscordClient::loadNextGuildIcon() {
  if (m_loadingGuildIconId.isEmpty() && !m_pendingGuildIcons.isEmpty()) {
    QVariantMap request = m_pendingGuildIcons.dequeue();
    m_loadingGuildIconId = request.value("guildId").toString();
    m_queuedGuildIconIds.removeAll(m_loadingGuildIconId);
    m_guildIconClient.downloadGuildIcon(m_loadingGuildIconId,
                                        request.value("iconHash").toString(),
                                        request.value("path").toString());
  }

  if (m_loadingGuildIconId2.isEmpty() && !m_pendingGuildIcons.isEmpty()) {
    QVariantMap request = m_pendingGuildIcons.dequeue();
    m_loadingGuildIconId2 = request.value("guildId").toString();
    m_queuedGuildIconIds.removeAll(m_loadingGuildIconId2);
    m_guildIconClient2.downloadGuildIcon(m_loadingGuildIconId2,
                                         request.value("iconHash").toString(),
                                         request.value("path").toString());
  }
}

QString DiscordClient::avatarCachePath(const DiscordUser &user) const {
  QDir dir(QDir::homePath());
  dir.mkpath("data/avatar-cache");
  return dir.absoluteFilePath(
      QString("data/avatar-cache/%1_%2.png").arg(user.id).arg(user.avatarHash));
}

QString DiscordClient::guildIconCachePath(const QString &guildId,
                                          const QString &iconHash) const {
  QDir dir(QDir::homePath());
  dir.mkpath("data/guild-icon-cache");
  return dir.absoluteFilePath(
      QString("data/guild-icon-cache/%1_%2.png").arg(guildId).arg(iconHash));
}

QString DiscordClient::avatarSourceForPath(const QString &path) const {
  QString cleanPath = QDir::fromNativeSeparators(path);
  if (cleanPath.startsWith("/")) {
    return QString("file://%1").arg(cleanPath);
  }

  return QString("file:///%1").arg(cleanPath);
}

QVariantMap DiscordClient::guildToItem(const QVariantMap &guild) const {
  QVariantMap item;
  QString name = guild.value("name").toString();
  QString guildId = guild.value("id").toString();
  QString iconHash = guild.value("icon").toString();
  item["type"] = "server";
  item["id"] = guildId;
  item["name"] = name;
  item["unreadCount"] = guild.value("mention_count").toInt();
  if (guild.contains("position")) {
    item["position"] = guild.value("position").toInt();
  }
  item["initials"] = firstTwoWordLetters(name);
  item["iconHash"] = iconHash;
  item["icon"] = QString();
  if (!guildId.isEmpty() && !iconHash.isEmpty()) {
    QString path = guildIconCachePath(guildId, iconHash);
    QFileInfo cachedFile(path);
    if (cachedFile.exists() && cachedFile.size() > 0) {
      item["icon"] = avatarSourceForPath(path);
    }
  }
  return item;
}

bool DiscordClient::applyGuildOrder(const QStringList &orderedGuildIds) {
  if (orderedGuildIds.isEmpty() || m_guilds.isEmpty()) {
    return false;
  }

  QVariantList ordered;
  QStringList usedIds;
  for (int i = 0; i < orderedGuildIds.size(); ++i) {
    QString guildId = orderedGuildIds.at(i).trimmed();
    if (guildId.isEmpty() || usedIds.contains(guildId)) {
      continue;
    }

    for (int j = 0; j < m_guilds.size(); ++j) {
      QVariantMap guild = m_guilds.at(j).toMap();
      if (guild.value("id").toString() == guildId) {
        ordered.append(guild);
        usedIds.append(guildId);
        break;
      }
    }
  }

  if (ordered.isEmpty()) {
    return false;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (!usedIds.contains(guild.value("id").toString())) {
      ordered.append(guild);
    }
  }

  m_guilds = ordered;
  return true;
}

bool DiscordClient::applyGuildOrderFromGatewayPayload(
    const QVariantMap &payload) {
  QStringList orderedGuildIds;
  appendGuildIdsFromUserSettingsProto(
      &orderedGuildIds, payload.value("user_settings_proto").toString());

  QVariantMap settings = payload.value("settings").toMap();
  appendGuildIdsFromUserSettingsProto(&orderedGuildIds,
                                      settings.value("proto").toString());

  QVariantList folders = payload.value("guild_folders").toList();
  if (folders.isEmpty()) {
    QVariantMap userSettings = payload.value("user_settings").toMap();
    folders = userSettings.value("guild_folders").toList();
  }

  for (int i = 0; i < folders.size(); ++i) {
    QVariantMap folder = folders.at(i).toMap();
    QVariantList guildIds = folder.value("guild_ids").toList();
    for (int j = 0; j < guildIds.size(); ++j) {
      appendUniqueGuildId(&orderedGuildIds, guildIds.at(j).toString());
    }
  }

  if (orderedGuildIds.isEmpty()) {
    QVariantList guilds = payload.value("guilds").toList();
    for (int i = 0; i < guilds.size(); ++i) {
      orderedGuildIds.append(guilds.at(i).toMap().value("id").toString());
    }
  }

  if (!orderedGuildIds.isEmpty()) {
    m_orderedGuildIds = orderedGuildIds;
  }

  return applyGuildOrder(orderedGuildIds);
}

void DiscordClient::sortGuilds() { applyGuildOrder(m_orderedGuildIds); }

void DiscordClient::updateGuildUnreadCount(const QString &guildId, int delta) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty() || delta == 0) {
    return;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      int count = guild.value("unreadCount").toInt() + delta;
      guild["unreadCount"] = count < 0 ? 0 : count;
      m_guilds.replace(i, guild);
      sortGuilds();
      if (m_store) {
        m_store->setGuilds(m_guilds);
      }
      return;
    }
  }
}

void DiscordClient::updateGuildChannelUnread(const QString &channelId,
                                             bool unread) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  bool changed = false;
  for (int i = 0; i < m_allGuildChannels.size(); ++i) {
    QVariantMap channel = m_allGuildChannels.at(i).toMap();
    if (channel.value("id").toString() == safeChannelId) {
      channel["unread"] = unread;
      m_allGuildChannels.replace(i, channel);
      changed = true;
      break;
    }
  }

  for (int i = 0; i < m_visibleGuildChannels.size(); ++i) {
    QVariantMap channel = m_visibleGuildChannels.at(i).toMap();
    if (channel.value("id").toString() == safeChannelId) {
      channel["unread"] = unread;
      m_visibleGuildChannels.replace(i, channel);
      changed = true;
      break;
    }
  }

  if (changed && m_store) {
    m_store->setGuildChannels(m_visibleGuildChannels);
  }
}

QVariantMap DiscordClient::dmChannelToItem(const QVariantMap &channel) const {
  QVariantMap item;
  QVariantList recipients = channel.value("recipients").toList();
  QVariantMap recipient =
      recipients.isEmpty() ? QVariantMap() : recipients.first().toMap();
  QString name = channel.value("name").toString();
  if (name.isEmpty()) {
    name = recipient.value("global_name").toString();
  }
  if (name.isEmpty()) {
    name = recipient.value("username").toString();
  }

  item["type"] = "dm";
  item["id"] = channel.value("id").toString();
  item["name"] = name;
  item["lastMessageId"] = channel.value("last_message_id").toString();
  item["initials"] = firstLetter(name);
  item["avatarColor"] = "#5865F2";
  item["avatarUserId"] = recipient.value("id").toString();
  item["avatarHash"] = recipient.value("avatar").toString();
  item["avatar"] = QString();
  if (!item.value("avatarUserId").toString().isEmpty() &&
      !item.value("avatarHash").toString().isEmpty()) {
    DiscordUser user;
    user.id = item.value("avatarUserId").toString();
    user.avatarHash = item.value("avatarHash").toString();
    QString path = avatarCachePath(user);
    QFileInfo cachedFile(path);
    if (cachedFile.exists() && cachedFile.size() > 0) {
      item["avatar"] = avatarSourceForPath(path);
    }
  }
  return item;
}

QVariantMap
DiscordClient::guildChannelToItem(const QVariantMap &channel) const {
  QVariantMap item;
  int type = channel.value("type").toInt();
  item["id"] = channel.value("id").toString();
  item["name"] = channel.value("name").toString();
  item["type"] = type == DiscordChannel::GuildCategory ? "category" : "channel";
  item["icon"] = type == DiscordChannel::GuildVoice
                     ? "asset:///images/icons/speaker.png"
                     : "asset:///images/icons/hash.png";
  item["position"] = channel.value("position").toInt();
  item["channelType"] = type;
  item["parentId"] = channel.value("parent_id").toString();
  item["unread"] = channel.value("unread").toBool();
  return item;
}

QVariantList DiscordClient::sortedAccessibleGuildChannels(
    const QVariantList &channels) const {
  QVariantList categories;
  QVariantList rootChannels;
  QMap<QString, QVariantList> channelsByCategory;
  QStringList categoryIds;

  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = channels.at(i).toMap();
    if (item.value("type").toString() == "category") {
      categories.append(item);
      categoryIds.append(item.value("id").toString());
    } else {
      QString parentId = item.value("parentId").toString();
      if (parentId.isEmpty() || !categoryIds.contains(parentId)) {
        rootChannels.append(item);
      } else {
        QVariantList categoryChannels = channelsByCategory.value(parentId);
        categoryChannels.append(item);
        channelsByCategory.insert(parentId, categoryChannels);
      }
    }
  }

  for (int i = 0; i < rootChannels.size(); ++i) {
    QVariantMap item = rootChannels.at(i).toMap();
    QString parentId = item.value("parentId").toString();
    if (!parentId.isEmpty() && categoryIds.contains(parentId)) {
      QVariantList categoryChannels = channelsByCategory.value(parentId);
      categoryChannels.append(item);
      channelsByCategory.insert(parentId, categoryChannels);
      rootChannels.removeAt(i);
      --i;
    }
  }

  stableSortItems(&categories, positionShouldMoveBefore);
  stableSortItems(&rootChannels, positionShouldMoveBefore);

  QVariantList result;
  int rootIndex = 0;
  for (int i = 0; i < categories.size(); ++i) {
    QVariantMap item = categories.at(i).toMap();

    while (rootIndex < rootChannels.size() &&
           positionShouldMoveBefore(rootChannels.at(rootIndex).toMap(), item)) {
      result.append(rootChannels.at(rootIndex));
      ++rootIndex;
    }

    result.append(item);

    QVariantList categoryChannels =
        channelsByCategory.value(item.value("id").toString());
    stableSortItems(&categoryChannels, positionShouldMoveBefore);
    for (int j = 0; j < categoryChannels.size(); ++j) {
      result.append(categoryChannels.at(j));
    }
  }

  while (rootIndex < rootChannels.size()) {
    result.append(rootChannels.at(rootIndex));
    ++rootIndex;
  }

  return result;
}

void DiscordClient::moveDmToTop(const QString &channelId,
                                const QString &lastMessageId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap item = m_allDmChannels.at(i).toMap();
    if (item.value("id").toString() == safeChannelId) {
      if (!lastMessageId.trimmed().isEmpty()) {
        item["lastMessageId"] = lastMessageId.trimmed();
      }
      m_allDmChannels.removeAt(i);
      m_allDmChannels.prepend(item);
      break;
    }
  }

  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap item = m_dmChannels.at(i).toMap();
    if (item.value("id").toString() == safeChannelId) {
      if (!lastMessageId.trimmed().isEmpty()) {
        item["lastMessageId"] = lastMessageId.trimmed();
      }
      m_dmChannels.removeAt(i);
      m_dmChannels.prepend(item);
      if (m_store) {
        m_store->setDmChannels(m_dmChannels);
      }
      return;
    }
  }
}

void DiscordClient::applyGatewayOrderingEvent(const QString &eventName,
                                              const QVariantMap &payload) {
  if (eventName == "MESSAGE_CREATE") {
    QString guildId = payload.value("guild_id").toString();
    QString channelId = payload.value("channel_id").toString();
    if (guildId.isEmpty() && !channelId.isEmpty()) {
      moveDmToTop(channelId, payload.value("id").toString());
    } else if (!guildId.isEmpty() &&
               payload.value("author").toMap().value("id").toString() !=
                   (m_store ? m_store->currentUserId() : QString())) {
      updateGuildUnreadCount(guildId, 1);
      updateGuildChannelUnread(channelId, true);
    }
    return;
  }

  if (eventName == "CHANNEL_CREATE" || eventName == "CHANNEL_DELETE" ||
      eventName == "GUILD_CREATE" || eventName == "GUILD_DELETE" ||
      eventName == "READY" || eventName == "READY_SUPPLEMENTAL" ||
      eventName == "USER_SETTINGS_PROTO_UPDATE") {
    applyGuildOrderFromGatewayPayload(payload);
    sortGuilds();
    stableSortItems(&m_allDmChannels, dmShouldMoveBefore);
    stableSortItems(&m_dmChannels, dmShouldMoveBefore);
    if (m_store) {
      m_store->setGuilds(m_guilds);
      m_store->setDmChannels(m_dmChannels);
    }
  }
}

void DiscordClient::appendVisibleGuildChannels() {
  int nextCount = m_visibleGuildChannelCount + kPageSize;
  if (nextCount > m_allGuildChannels.size()) {
    nextCount = m_allGuildChannels.size();
  }

  for (int i = m_visibleGuildChannelCount; i < nextCount; ++i) {
    m_visibleGuildChannels.append(m_allGuildChannels.at(i));
  }

  m_visibleGuildChannelCount = nextCount;
  m_guildChannelsHasMore =
      m_visibleGuildChannelCount < m_allGuildChannels.size();
  if (m_store) {
    m_store->setGuildChannels(m_visibleGuildChannels);
  }
}

void DiscordClient::updateDataLoading() {
  if (m_store) {
    m_store->setDataLoading(m_loadingGuilds || m_loadingDmChannels ||
                            m_loadingGuildChannels);
  }
}
