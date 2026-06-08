#include "Client.hpp"

#include "AppStore.hpp"
#include "AvatarCacheWorker.hpp"
#include "discord/DiscordUtils.hpp"
#include "discord/NetworkWorker.hpp"
#include "models/Models.hpp"

#include "client/AvatarManager.hpp"
#include "client/CacheManager.hpp"
#include "client/GatewayHandler.hpp"
#include "client/ItemMapper.hpp"
#include "client/SortUtils.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QMetaType>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QTimer>

namespace {
const int kPageSize = 25;
const int kGuildPageSize = 100;
} // namespace

DiscordClient::DiscordClient(QObject *parent)
    : QObject(parent), m_store(0), m_networkThread(0), m_networkWorker(0),
      m_avatarCacheThread(0), m_avatarCacheWorker(0), m_cacheManager(0),
      m_avatarManager(0), m_gatewayHandler(0), m_itemMapper(0), m_sortUtils(0),
      m_pendingAvatars(m_avatarState.pendingAvatars),
      m_pendingGuildIcons(m_avatarState.pendingGuildIcons),
      m_avatarCacheRequests(m_avatarState.avatarCacheRequests),
      m_guildIconCacheRequests(m_avatarState.guildIconCacheRequests),
      m_avatarSourcesByUserId(m_avatarState.avatarSourcesByUserId),
      m_dmChannelsById(m_state.dmChannelsById),
      m_dmChannelIdsByRecipientId(m_state.dmChannelIdsByRecipientId),
      m_allDmChannelIndexById(m_state.allDmChannelIndexById),
      m_visibleDmChannelIndexById(m_state.visibleDmChannelIndexById),
      m_loadingAvatarUserId(m_avatarState.loadingAvatarUserId),
      m_loadingAvatarUserId2(m_avatarState.loadingAvatarUserId2),
      m_loadingGuildIconId(m_avatarState.loadingGuildIconId),
      m_loadingGuildIconId2(m_avatarState.loadingGuildIconId2),
      m_queuedAvatarUserIds(m_avatarState.queuedAvatarUserIds),
      m_loadedAvatarUserIds(m_avatarState.loadedAvatarUserIds),
      m_queuedGuildIconIds(m_avatarState.queuedGuildIconIds),
      m_loadedGuildIconIds(m_avatarState.loadedGuildIconIds),
      m_orderedGuildIds(m_state.orderedGuildIds),
      m_dmPresenceByUserId(m_state.dmPresenceByUserId),
      m_lastGuildId(m_state.lastGuildId),
      m_lastDmChannelId(m_state.lastDmChannelId),
      m_selectedGuildId(m_state.selectedGuildId), m_guilds(m_state.guilds),
      m_allDmChannels(m_state.allDmChannels), m_dmChannels(m_state.dmChannels),
      m_allGuildChannels(m_state.allGuildChannels),
      m_visibleGuildChannels(m_state.visibleGuildChannels),
      m_pendingMentionCountsByGuildId(m_state.pendingMentionCountsByGuildId),
      m_pendingUnreadGuildIds(m_state.pendingUnreadGuildIds),
      m_pendingUnreadChannelIds(m_state.pendingUnreadChannelIds),
      m_pendingDmPresenceUserIds(m_state.pendingDmPresenceUserIds),
      m_visibleDmChannelCount(m_state.visibleDmChannelCount),
      m_visibleGuildChannelCount(m_state.visibleGuildChannelCount),
      m_loadingGuilds(m_state.loadingGuilds),
      m_loadingDmChannels(m_state.loadingDmChannels),
      m_loadingGuildChannels(m_state.loadingGuildChannels),
      m_guildsHasMore(m_state.guildsHasMore),
      m_dmChannelsHasMore(m_state.dmChannelsHasMore),
      m_guildChannelsHasMore(m_state.guildChannelsHasMore), m_loggedIn(false),
      m_busy(false), m_gatewayUiUpdateQueued(m_state.gatewayUiUpdateQueued),
      m_pendingDmUiUpdate(m_state.pendingDmUiUpdate),
      m_dmCacheSaveQueued(m_state.dmCacheSaveQueued),
      m_bootstrapCacheLoaded(m_state.bootstrapCacheLoaded),
      m_statusText("Disconnected") {
  initializeNetworkWorker();
  initializeAvatarCacheWorker();
  initializeManagers();

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::DiscordClient(AppStore *store, QObject *parent)
    : QObject(parent), m_store(store), m_networkThread(0), m_networkWorker(0),
      m_avatarCacheThread(0), m_avatarCacheWorker(0), m_cacheManager(0),
      m_avatarManager(0), m_gatewayHandler(0), m_itemMapper(0), m_sortUtils(0),
      m_pendingAvatars(m_avatarState.pendingAvatars),
      m_pendingGuildIcons(m_avatarState.pendingGuildIcons),
      m_avatarCacheRequests(m_avatarState.avatarCacheRequests),
      m_guildIconCacheRequests(m_avatarState.guildIconCacheRequests),
      m_avatarSourcesByUserId(m_avatarState.avatarSourcesByUserId),
      m_dmChannelsById(m_state.dmChannelsById),
      m_dmChannelIdsByRecipientId(m_state.dmChannelIdsByRecipientId),
      m_allDmChannelIndexById(m_state.allDmChannelIndexById),
      m_visibleDmChannelIndexById(m_state.visibleDmChannelIndexById),
      m_loadingAvatarUserId(m_avatarState.loadingAvatarUserId),
      m_loadingAvatarUserId2(m_avatarState.loadingAvatarUserId2),
      m_loadingGuildIconId(m_avatarState.loadingGuildIconId),
      m_loadingGuildIconId2(m_avatarState.loadingGuildIconId2),
      m_queuedAvatarUserIds(m_avatarState.queuedAvatarUserIds),
      m_loadedAvatarUserIds(m_avatarState.loadedAvatarUserIds),
      m_queuedGuildIconIds(m_avatarState.queuedGuildIconIds),
      m_loadedGuildIconIds(m_avatarState.loadedGuildIconIds),
      m_orderedGuildIds(m_state.orderedGuildIds),
      m_dmPresenceByUserId(m_state.dmPresenceByUserId),
      m_lastGuildId(m_state.lastGuildId),
      m_lastDmChannelId(m_state.lastDmChannelId),
      m_selectedGuildId(m_state.selectedGuildId), m_guilds(m_state.guilds),
      m_allDmChannels(m_state.allDmChannels), m_dmChannels(m_state.dmChannels),
      m_allGuildChannels(m_state.allGuildChannels),
      m_visibleGuildChannels(m_state.visibleGuildChannels),
      m_pendingMentionCountsByGuildId(m_state.pendingMentionCountsByGuildId),
      m_pendingUnreadGuildIds(m_state.pendingUnreadGuildIds),
      m_pendingUnreadChannelIds(m_state.pendingUnreadChannelIds),
      m_pendingDmPresenceUserIds(m_state.pendingDmPresenceUserIds),
      m_visibleDmChannelCount(m_state.visibleDmChannelCount),
      m_visibleGuildChannelCount(m_state.visibleGuildChannelCount),
      m_loadingGuilds(m_state.loadingGuilds),
      m_loadingDmChannels(m_state.loadingDmChannels),
      m_loadingGuildChannels(m_state.loadingGuildChannels),
      m_guildsHasMore(m_state.guildsHasMore),
      m_dmChannelsHasMore(m_state.dmChannelsHasMore),
      m_guildChannelsHasMore(m_state.guildChannelsHasMore), m_loggedIn(false),
      m_busy(false), m_gatewayUiUpdateQueued(m_state.gatewayUiUpdateQueued),
      m_pendingDmUiUpdate(m_state.pendingDmUiUpdate),
      m_dmCacheSaveQueued(m_state.dmCacheSaveQueued),
      m_bootstrapCacheLoaded(m_state.bootstrapCacheLoaded),
      m_statusText("Disconnected") {
  initializeNetworkWorker();
  initializeAvatarCacheWorker();
  initializeManagers();

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::~DiscordClient() {
  shutdownAvatarCacheWorker();
  shutdownNetworkWorker();
}

void DiscordClient::initializeManagers() {
  m_cacheManager = new CacheManager(this, m_store, this);
  m_avatarManager =
      new AvatarManager(this, m_networkWorker, m_avatarCacheWorker, this);
  m_gatewayHandler = new GatewayHandler(this, m_store, this);
  m_itemMapper = new ItemMapper(this);
  m_sortUtils = new SortUtils(this);
}

void DiscordClient::initializeNetworkWorker() {
  qRegisterMetaType<QVariantMap>("QVariantMap");
  qRegisterMetaType<QVariantList>("QVariantList");

  if (m_networkThread != 0 || m_networkWorker != 0) {
    return;
  }

  m_networkThread = new QThread(this);
  m_networkWorker = new DiscordNetworkWorker();
  m_networkWorker->moveToThread(m_networkThread);

  connect(m_networkThread, SIGNAL(finished()), m_networkWorker,
          SLOT(deleteLater()));
  connect(m_networkWorker, SIGNAL(loginSucceeded(QVariantMap)), this,
          SLOT(onRestLoginSucceeded(QVariantMap)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(loginFailed(QString)), this,
          SLOT(onRestLoginFailed(QString)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(guildsLoaded(QVariantList)), this,
          SLOT(onGuildsLoaded(QVariantList)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(dmChannelsLoaded(QVariantList)), this,
          SLOT(onDmChannelsLoaded(QVariantList)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(guildChannelsLoaded(QString, QVariantList)),
          this, SLOT(onGuildChannelsLoaded(QString, QVariantList)),
          Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(requestFailed(QString)), this,
          SLOT(onDataRequestFailed(QString)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(avatarDownloaded(QString, QString)), this,
          SLOT(onAvatarDownloaded(QString, QString)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(avatarDownloadFailed(QString, QString)), this,
          SLOT(onAvatarDownloadFailed(QString, QString)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(guildIconDownloaded(QString, QString)), this,
          SLOT(onGuildIconDownloaded(QString, QString)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(guildIconDownloadFailed(QString, QString)),
          this, SLOT(onGuildIconDownloadFailed(QString, QString)),
          Qt::QueuedConnection);
  connect(m_networkWorker,
          SIGNAL(gatewayDispatchReceived(QString, QVariantMap)), this,
          SLOT(onGatewayDispatch(QString, QVariantMap)), Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(cancelAllFinished()), m_networkThread,
          SLOT(quit()), Qt::DirectConnection);

  m_networkThread->start();
}

void DiscordClient::initializeAvatarCacheWorker() {
  if (m_avatarCacheThread != 0 || m_avatarCacheWorker != 0) {
    return;
  }

  m_avatarCacheThread = new QThread(this);
  m_avatarCacheWorker = new AvatarCacheWorker();
  m_avatarCacheWorker->moveToThread(m_avatarCacheThread);

  connect(m_avatarCacheThread, SIGNAL(finished()), m_avatarCacheWorker,
          SLOT(deleteLater()));
  connect(m_avatarCacheWorker, SIGNAL(avatarCacheHit(QString, QString)), this,
          SLOT(onAvatarCacheHit(QString, QString)), Qt::QueuedConnection);
  connect(m_avatarCacheWorker, SIGNAL(avatarCacheMiss(QString, QString)), this,
          SLOT(onAvatarCacheMiss(QString, QString)), Qt::QueuedConnection);
  connect(m_avatarCacheWorker, SIGNAL(guildIconCacheHit(QString, QString)),
          this, SLOT(onGuildIconCacheHit(QString, QString)),
          Qt::QueuedConnection);
  connect(m_avatarCacheWorker, SIGNAL(guildIconCacheMiss(QString, QString)),
          this, SLOT(onGuildIconCacheMiss(QString, QString)),
          Qt::QueuedConnection);

  m_avatarCacheThread->start();
}

void DiscordClient::shutdownAvatarCacheWorker() {
  m_avatarCacheRequests.clear();
  m_guildIconCacheRequests.clear();

  if (m_avatarCacheThread != 0) {
    if (m_avatarCacheThread->isRunning()) {
      m_avatarCacheThread->quit();
      m_avatarCacheThread->wait();
    }
    delete m_avatarCacheThread;
    m_avatarCacheThread = 0;
  } else if (m_avatarCacheWorker != 0) {
    delete m_avatarCacheWorker;
  }

  m_avatarCacheWorker = 0;
}

void DiscordClient::shutdownNetworkWorker() {
  if (m_networkWorker != 0 && m_networkThread != 0 &&
      m_networkThread->isRunning()) {
    QMetaObject::invokeMethod(m_networkWorker, "cancelAll",
                              Qt::QueuedConnection);
    m_networkThread->wait();
  } else if (m_networkWorker != 0) {
    delete m_networkWorker;
  }

  m_networkWorker = 0;

  if (m_networkThread != 0) {
    if (!m_networkThread->isFinished()) {
      m_networkThread->quit();
      m_networkThread->wait();
    }
    delete m_networkThread;
    m_networkThread = 0;
  }
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
  if (m_networkWorker == 0 || m_networkThread == 0) {
    initializeNetworkWorker();
  }
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "loginWithToken",
                              Qt::QueuedConnection,
                              Q_ARG(QString, trimmedToken));
  }
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
  m_cacheManager->clearBootstrapCache();
  shutdownNetworkWorker();
  m_avatarCacheRequests.clear();
  m_guildIconCacheRequests.clear();
  m_avatarSourcesByUserId.clear();
  m_dmChannelsById.clear();
  m_dmChannelIdsByRecipientId.clear();
  m_allDmChannelIndexById.clear();
  m_visibleDmChannelIndexById.clear();
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
  m_dmPresenceByUserId.clear();
  m_pendingDmPresenceUserIds.clear();
  m_guilds.clear();
  m_allDmChannels.clear();
  m_dmChannels.clear();
  m_allGuildChannels.clear();
  m_visibleGuildChannels.clear();
  m_pendingUnreadGuildIds.clear();
  m_pendingMentionCountsByGuildId.clear();
  m_pendingUnreadChannelIds.clear();
  m_gatewayUiUpdateQueued = false;
  m_pendingDmUiUpdate = false;
  m_dmCacheSaveQueued = false;
  m_bootstrapCacheLoaded = false;
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
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "fetchGuilds",
                              Qt::QueuedConnection, Q_ARG(QString, m_token),
                              Q_ARG(int, kGuildPageSize),
                              Q_ARG(QString, m_lastGuildId));
  }
}

void DiscordClient::loadGuildIcon(const QString &guildId) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      m_avatarManager->queueGuildIcon(
          safeGuildId, guild.value("iconHash").toString(), m_loadedGuildIconIds,
          m_guildIconCacheRequests, m_queuedGuildIconIds, m_loadingGuildIconId,
          m_loadingGuildIconId2, m_pendingGuildIcons);
      m_avatarManager->loadNextGuildIcon(
          m_loadingGuildIconId, m_loadingGuildIconId2, m_pendingGuildIcons,
          m_queuedGuildIconIds);
      return;
    }
  }
}

void DiscordClient::loadMoreDmChannels() {
  if (m_loadingGuilds || m_loadingDmChannels || !m_dmChannelsHasMore ||
      m_token.trimmed().isEmpty()) {
    return;
  }

  if (m_visibleDmChannelCount < m_allDmChannels.size()) {
    int nextCount = m_visibleDmChannelCount + kPageSize;
    if (nextCount > m_allDmChannels.size()) {
      nextCount = m_allDmChannels.size();
    }

    QVariantList appendedChannels;
    for (int i = m_visibleDmChannelCount; i < nextCount; ++i) {
      QVariantMap channel = m_allDmChannels.at(i).toMap();
      m_dmChannels.append(channel);
      appendedChannels.append(channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
        m_visibleDmChannelIndexById.insert(channelId, m_dmChannels.size() - 1);
        indexDmChannelRecipients(channel);
      }
      m_lastDmChannelId = channelId;
    }

    m_visibleDmChannelCount = nextCount;
    m_dmChannelsHasMore = m_visibleDmChannelCount < m_allDmChannels.size();
    if (m_store) {
      m_store->appendDmChannels(appendedChannels);
    }
    return;
  }

  m_loadingDmChannels = true;
  updateDataLoading();
  setStatusText("Loading DMs...");
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "fetchDmChannels",
                              Qt::QueuedConnection, Q_ARG(QString, m_token),
                              Q_ARG(int, kPageSize),
                              Q_ARG(QString, m_lastDmChannelId));
  }
}

void DiscordClient::loadDmAvatar(const QString &channelId) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  QVariantMap channel = m_dmChannelsById.value(safeChannelId).toMap();
  if (channel.isEmpty()) {
    return;
  }

  m_avatarManager->queueDmAvatar(
      safeChannelId, channel.value("avatarUserId").toString(),
      channel.value("avatarHash").toString(), m_loadedAvatarUserIds,
      m_avatarCacheRequests, m_queuedAvatarUserIds, m_loadingAvatarUserId,
      m_loadingAvatarUserId2, m_pendingAvatars);
  m_avatarManager->queueDmAvatar(
      safeChannelId, channel.value("avatarUserId2").toString(),
      channel.value("avatarHash2").toString(), m_loadedAvatarUserIds,
      m_avatarCacheRequests, m_queuedAvatarUserIds, m_loadingAvatarUserId,
      m_loadingAvatarUserId2, m_pendingAvatars);
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
}

void DiscordClient::loadGuildChannels(const QString &guildId) {
  if (m_loadingGuilds || m_loadingDmChannels || guildId.trimmed().isEmpty() ||
      m_token.trimmed().isEmpty()) {
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
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "fetchGuildChannels",
                              Qt::QueuedConnection, Q_ARG(QString, m_token),
                              Q_ARG(QString, m_selectedGuildId),
                              Q_ARG(int, kPageSize), Q_ARG(QString, QString()));
  }
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
  scheduleDmChannelsCacheSave();
  if (m_dmChannels.isEmpty()) {
    loadMoreDmChannels();
  }
}

void DiscordClient::selectGuild(const QString &guildId) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  bool sameGuild = safeGuildId == m_selectedGuildId;

  if (m_store) {
    m_store->selectGuild(safeGuildId);
  }
  scheduleDmChannelsCacheSave();
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
  saveGuildsCache();
  saveDmChannelsCache();
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
    m_avatarManager->loadCurrentUserAvatar(
        currentUser, m_avatarCacheRequests, m_loadingAvatarUserId,
        m_loadingAvatarUserId2, m_pendingAvatars, m_queuedAvatarUserIds);
  }

  m_cacheManager->loadBootstrapCache(
      m_guilds, m_allDmChannels, m_avatarSourcesByUserId, m_selectedGuildId,
      m_bootstrapCacheLoaded, m_guildsHasMore, m_lastGuildId,
      m_visibleDmChannelCount, m_lastDmChannelId, m_dmChannelsHasMore);
  if (m_bootstrapCacheLoaded && !m_allDmChannels.isEmpty()) {
    m_dmChannels.clear();
    m_dmChannelsById.clear();
    m_allDmChannelIndexById.clear();
    m_visibleDmChannelIndexById.clear();
    rebuildDmRecipientIndex();
    rebuildDmChannelIndexes();
    loadMoreDmChannels();
  }

  saveToken();
  setBusy(false);
  setLoggedIn(true);
  setStatusText("Connected");
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "connectGateway",
                              Qt::QueuedConnection, Q_ARG(QString, m_token));
  }
  loadGuilds();
  emit loginSucceeded();
}

void DiscordClient::onRestLoginFailed(const QString &message) {
  qDebug() << "[discord-client] REST login failed" << message;
  setBusy(false);
  setLoggedIn(false);
  if (m_networkWorker != 0) {
    QMetaObject::invokeMethod(m_networkWorker, "disconnectGateway",
                              Qt::QueuedConnection);
  }
  setStatusText(message);
  emit loginFailed(message);
}

void DiscordClient::onGuildsLoaded(const QVariantList &guilds) {
  QVariantMap existingIconsByGuildId;
  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap existingGuild = m_guilds.at(i).toMap();
    QString guildId = existingGuild.value("id").toString();
    QString icon = existingGuild.value("icon").toString();
    if (!guildId.isEmpty() && !icon.isEmpty()) {
      existingIconsByGuildId.insert(guildId, icon);
    }
  }

  if (m_bootstrapCacheLoaded) {
    m_guilds.clear();
    m_lastGuildId.clear();
    m_guildsHasMore = true;
    m_bootstrapCacheLoaded = false;
  }

  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    QVariantMap item = m_itemMapper->guildToItem(guild);
    if (!item.value("id").toString().isEmpty()) {
      QString existingIcon =
          existingIconsByGuildId.value(item.value("id").toString()).toString();
      if (!existingIcon.isEmpty()) {
        item["icon"] = existingIcon;
      }
      m_guilds.append(item);
      m_lastGuildId = item.value("id").toString();
    }
  }

  if (guilds.size() >= kGuildPageSize && !m_lastGuildId.isEmpty()) {
    m_guildsHasMore = true;
    setStatusText("Loading servers...");
    if (m_networkWorker != 0) {
      QMetaObject::invokeMethod(m_networkWorker, "fetchGuilds",
                                Qt::QueuedConnection, Q_ARG(QString, m_token),
                                Q_ARG(int, kGuildPageSize),
                                Q_ARG(QString, m_lastGuildId));
    }
    return;
  }

  m_loadingGuilds = false;
  m_guildsHasMore = false;
  updateDataLoading();

  sortGuilds();

  if (m_store && !m_orderedGuildIds.isEmpty()) {
    m_store->setGuilds(m_guilds);
    saveGuildsCache();
  } else {
    qDebug() << "[discord-client] guild REST sync ready; waiting for gateway"
             << "guild order before updating UI";
  }
  setStatusText("Connected");
  if (!m_loadingDmChannels && m_token.trimmed().isEmpty() == false) {
    m_lastDmChannelId.clear();
    m_dmChannelsHasMore = true;
    m_loadingDmChannels = true;
    updateDataLoading();
    setStatusText("Loading DMs...");
    if (m_networkWorker != 0) {
      QMetaObject::invokeMethod(m_networkWorker, "fetchDmChannels",
                                Qt::QueuedConnection, Q_ARG(QString, m_token),
                                Q_ARG(int, kPageSize),
                                Q_ARG(QString, QString()));
    }
  }
}

void DiscordClient::onDmChannelsLoaded(const QVariantList &channels) {
  m_loadingDmChannels = false;
  updateDataLoading();
  QVariantMap existingAvatarsByUserId;
  for (QVariantMap::const_iterator it = m_avatarSourcesByUserId.constBegin();
       it != m_avatarSourcesByUserId.constEnd(); ++it) {
    if (!it.key().isEmpty() && !it.value().toString().isEmpty()) {
      existingAvatarsByUserId.insert(it.key(), it.value());
    }
  }
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap existingChannel = m_allDmChannels.at(i).toMap();
    QString userId = existingChannel.value("avatarUserId").toString();
    QString avatar = existingChannel.value("avatar").toString();
    if (!userId.isEmpty() && !avatar.isEmpty()) {
      existingAvatarsByUserId.insert(userId, avatar);
    }
    QString userId2 = existingChannel.value("avatarUserId2").toString();
    QString avatar2 = existingChannel.value("avatar2").toString();
    if (!userId2.isEmpty() && !avatar2.isEmpty()) {
      existingAvatarsByUserId.insert(userId2, avatar2);
    }
  }

  bool changed = false;
  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = m_itemMapper->dmChannelToItem(
        channels.at(i).toMap(), existingAvatarsByUserId, m_dmPresenceByUserId);
    QString channelId = item.value("id").toString();
    if (!channelId.isEmpty()) {
      QString avatar =
          existingAvatarsByUserId.value(item.value("avatarUserId").toString())
              .toString();
      if (!avatar.isEmpty()) {
        item["avatar"] = avatar;
      }
      QString avatar2 =
          existingAvatarsByUserId.value(item.value("avatarUserId2").toString())
              .toString();
      if (!avatar2.isEmpty()) {
        item["avatar2"] = avatar2;
      }

      int existingIndex = m_allDmChannelIndexById.value(channelId, -1).toInt();
      if (existingIndex >= 0 && existingIndex < m_allDmChannels.size()) {
        QVariantMap existingChannel = m_allDmChannels.at(existingIndex).toMap();
        item["status"] = existingChannel.value("status");
        item["statusColor"] = existingChannel.value("statusColor");
        item["unread"] = existingChannel.value("unread");
        m_allDmChannels.replace(existingIndex, item);

        int visibleIndex =
            m_visibleDmChannelIndexById.value(channelId, -1).toInt();
        if (visibleIndex >= 0 && visibleIndex < m_dmChannels.size()) {
          m_dmChannels.replace(visibleIndex, item);
          m_dmChannelsById.insert(channelId, item);
          changed = true;
        }
      } else {
        m_allDmChannels.append(item);
        m_allDmChannelIndexById.insert(channelId, m_allDmChannels.size() - 1);
        indexDmChannelRecipients(item);
        changed = true;
      }
    }
  }

  if (changed) {
    sortDmChannels();
    rebuildDmChannelIndexes();
    if (m_store) {
      m_store->setDmChannels(m_dmChannels);
    }
  }

  m_dmChannelsHasMore = !channels.isEmpty();
  if (m_dmChannelsHasMore) {
    loadMoreDmChannels();
  }
  saveDmChannelsCache();
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
    QVariantMap item = m_itemMapper->guildChannelToItem(channels.at(i).toMap());
    if (!item.value("id").toString().isEmpty()) {
      rawChannels.append(item);
    }
  }
  m_allGuildChannels = m_sortUtils->sortedAccessibleGuildChannels(rawChannels);

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
  QString source = m_avatarManager->avatarSourceForPath(localPath);
  m_avatarSourcesByUserId.insert(userId, source);
  if (m_store && userId == m_store->currentUserId()) {
    m_store->setCurrentUserAvatarSource(source);
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap channel = m_allDmChannels.at(i).toMap();
    if (channel.value("avatarUserId").toString() == userId) {
      channel["avatar"] = source;
      m_allDmChannels.replace(i, channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
      }
    } else if (channel.value("avatarUserId2").toString() == userId) {
      channel["avatar2"] = source;
      m_allDmChannels.replace(i, channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
      }
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
    } else if (channel.value("avatarUserId2").toString() == userId) {
      channel["avatar2"] = source;
      m_dmChannels.replace(i, channel);
      if (m_store) {
        m_store->updateDmAvatar2(channel.value("id").toString(), source);
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
  scheduleDmChannelsCacheSave();
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
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
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
}

void DiscordClient::onGuildIconDownloaded(const QString &guildId,
                                          const QString &localPath) {
  QString source = m_avatarManager->avatarSourceForPath(localPath);
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
  saveGuildsCache();
  m_avatarManager->loadNextGuildIcon(m_loadingGuildIconId,
                                     m_loadingGuildIconId2, m_pendingGuildIcons,
                                     m_queuedGuildIconIds);
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
  m_avatarManager->loadNextGuildIcon(m_loadingGuildIconId,
                                     m_loadingGuildIconId2, m_pendingGuildIcons,
                                     m_queuedGuildIconIds);
}

void DiscordClient::onAvatarCacheHit(const QString &userId,
                                     const QString &path) {
  m_avatarCacheRequests.remove(userId);
  QString source = m_avatarManager->avatarSourceForPath(path);
  m_avatarSourcesByUserId.insert(userId, source);
  if (m_store && userId == m_store->currentUserId()) {
    m_store->setCurrentUserAvatarSource(source);
  }

  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QVariantMap channel = m_allDmChannels.at(i).toMap();
    if (channel.value("avatarUserId").toString() == userId) {
      channel["avatar"] = source;
      m_allDmChannels.replace(i, channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
      }
    } else if (channel.value("avatarUserId2").toString() == userId) {
      channel["avatar2"] = source;
      m_allDmChannels.replace(i, channel);
      QString channelId = channel.value("id").toString();
      if (!channelId.isEmpty()) {
        m_dmChannelsById.insert(channelId, channel);
      }
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
    } else if (channel.value("avatarUserId2").toString() == userId) {
      channel["avatar2"] = source;
      m_dmChannels.replace(i, channel);
      if (m_store) {
        m_store->updateDmAvatar2(channel.value("id").toString(), source);
      }
    }
  }

  if (!m_loadedAvatarUserIds.contains(userId)) {
    m_loadedAvatarUserIds.append(userId);
  }
}

void DiscordClient::onAvatarCacheMiss(const QString &userId,
                                      const QString &path) {
  QString avatarHash = m_avatarCacheRequests.value(userId).toString();
  m_avatarCacheRequests.remove(userId);
  if (userId.trimmed().isEmpty() || avatarHash.trimmed().isEmpty()) {
    return;
  }

  QVariantMap request;
  request["channelId"] = QString();
  request["userId"] = userId;
  request["avatarHash"] = avatarHash;
  request["path"] = path;
  m_pendingAvatars.enqueue(request);
  if (!m_queuedAvatarUserIds.contains(userId)) {
    m_queuedAvatarUserIds.append(userId);
  }
  m_avatarManager->loadNextAvatar(m_loadingAvatarUserId, m_loadingAvatarUserId2,
                                  m_pendingAvatars, m_queuedAvatarUserIds);
}

void DiscordClient::onGuildIconCacheHit(const QString &guildId,
                                        const QString &path) {
  m_guildIconCacheRequests.remove(guildId);
  QString source = m_avatarManager->avatarSourceForPath(path);
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
}

void DiscordClient::onGuildIconCacheMiss(const QString &guildId,
                                         const QString &path) {
  QString iconHash = m_guildIconCacheRequests.value(guildId).toString();
  m_guildIconCacheRequests.remove(guildId);
  if (guildId.trimmed().isEmpty() || iconHash.trimmed().isEmpty()) {
    return;
  }

  QVariantMap request;
  request["guildId"] = guildId;
  request["iconHash"] = iconHash;
  request["path"] = path;
  m_pendingGuildIcons.enqueue(request);
  if (!m_queuedGuildIconIds.contains(guildId)) {
    m_queuedGuildIconIds.append(guildId);
  }
  m_avatarManager->loadNextGuildIcon(m_loadingGuildIconId,
                                     m_loadingGuildIconId2, m_pendingGuildIcons,
                                     m_queuedGuildIconIds);
}

void DiscordClient::onGatewayDispatch(const QString &eventName,
                                      const QVariantMap &payload) {
  m_gatewayHandler->applyGatewayOrderingEvent(
      eventName, payload, m_pendingUnreadGuildIds,
      m_pendingMentionCountsByGuildId, m_pendingUnreadChannelIds,
      m_pendingDmUiUpdate, m_gatewayUiUpdateQueued);
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

void DiscordClient::saveGuildsCache() const {
  m_cacheManager->saveGuildsCache(m_guilds);
}

void DiscordClient::saveDmChannelsCache() const {
  m_cacheManager->saveDmChannelsCache(m_allDmChannels);
}

void DiscordClient::scheduleDmChannelsCacheSave() {
  if (m_dmCacheSaveQueued) {
    return;
  }

  m_dmCacheSaveQueued = true;
  QTimer::singleShot(1000, this, SLOT(savePendingDmChannelsCache()));
}

void DiscordClient::savePendingDmChannelsCache() {
  m_dmCacheSaveQueued = false;
  saveDmChannelsCache();
}

bool DiscordClient::applyGuildOrderFromGatewayPayload(
    const QVariantMap &payload) {
  return m_sortUtils->applyGuildOrderFromGatewayPayload(
      m_guilds, m_orderedGuildIds, payload);
}

void DiscordClient::sortGuilds() {
  m_sortUtils->applyGuildOrder(m_guilds, m_orderedGuildIds);
}

void DiscordClient::sortDmChannels() {
  DiscordUtils::stableSortItems(&m_allDmChannels,
                                DiscordUtils::dmShouldMoveBefore);
  DiscordUtils::stableSortItems(&m_dmChannels,
                                DiscordUtils::dmShouldMoveBefore);
}

int DiscordClient::guildMentionCount(const QString &guildId) const {
  QString safeGuildId = guildId.trimmed();
  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      return guild.value("mentionCount").toInt();
    }
  }
  return 0;
}

void DiscordClient::updateGuildMentionCount(const QString &guildId,
                                            int mentionCount) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      guild["mentionCount"] = mentionCount < 0 ? 0 : mentionCount;
      m_guilds.replace(i, guild);
      return;
    }
  }
}

void DiscordClient::updateGuildUnread(const QString &guildId, bool unread) {
  QString safeGuildId = guildId.trimmed();
  if (safeGuildId.isEmpty()) {
    return;
  }

  for (int i = 0; i < m_guilds.size(); ++i) {
    QVariantMap guild = m_guilds.at(i).toMap();
    if (guild.value("id").toString() == safeGuildId) {
      guild["unread"] = unread;
      m_guilds.replace(i, guild);
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

  Q_UNUSED(changed);
}

void DiscordClient::indexDmChannelRecipients(const QVariantMap &channel) {
  QString channelId = channel.value("id").toString();
  if (channelId.isEmpty()) {
    return;
  }

  QVariantList recipientIds = channel.value("recipientIds").toList();
  for (int i = 0; i < recipientIds.size(); ++i) {
    QString userId = recipientIds.at(i).toString().trimmed();
    if (userId.isEmpty()) {
      continue;
    }

    QStringList channelIds =
        m_dmChannelIdsByRecipientId.value(userId).toStringList();
    if (!channelIds.contains(channelId)) {
      channelIds.append(channelId);
      m_dmChannelIdsByRecipientId.insert(userId, channelIds);
    }
  }
}

void DiscordClient::rebuildDmRecipientIndex() {
  m_dmChannelIdsByRecipientId.clear();
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    indexDmChannelRecipients(m_allDmChannels.at(i).toMap());
  }
}

void DiscordClient::rebuildDmChannelIndexes() {
  m_allDmChannelIndexById.clear();
  for (int i = 0; i < m_allDmChannels.size(); ++i) {
    QString channelId = m_allDmChannels.at(i).toMap().value("id").toString();
    if (!channelId.isEmpty()) {
      m_allDmChannelIndexById.insert(channelId, i);
    }
  }

  m_dmChannelsById.clear();
  m_visibleDmChannelIndexById.clear();
  for (int i = 0; i < m_dmChannels.size(); ++i) {
    QVariantMap channel = m_dmChannels.at(i).toMap();
    QString channelId = channel.value("id").toString();
    if (!channelId.isEmpty()) {
      m_dmChannelsById.insert(channelId, channel);
      m_visibleDmChannelIndexById.insert(channelId, i);
    }
  }
}

void DiscordClient::updateDmPresence(const QString &userId,
                                     const QString &status) {
  QString safeUserId = userId.trimmed();
  QString safeStatus = status.trimmed();
  if (safeUserId.isEmpty()) {
    return;
  }

  if (safeStatus.isEmpty()) {
    safeStatus = "offline";
  }

  if (m_dmPresenceByUserId.contains(safeUserId) &&
      m_dmPresenceByUserId.value(safeUserId).toString() == safeStatus) {
    return;
  }

  m_dmPresenceByUserId.insert(safeUserId, safeStatus);
  if (!m_pendingDmPresenceUserIds.contains(safeUserId)) {
    m_pendingDmPresenceUserIds.append(safeUserId);
  }
}

bool DiscordClient::applyPendingDmPresences() {
  if (m_pendingDmPresenceUserIds.isEmpty()) {
    return false;
  }

  QStringList userIds = m_pendingDmPresenceUserIds;
  m_pendingDmPresenceUserIds.clear();
  QStringList affectedChannelIds;
  QSet<QString> affectedChannelIdSet;

  for (int userIndex = 0; userIndex < userIds.size(); ++userIndex) {
    QStringList channelIds =
        m_dmChannelIdsByRecipientId.value(userIds.at(userIndex)).toStringList();
    for (int channelIndex = 0; channelIndex < channelIds.size();
         ++channelIndex) {
      QString channelId = channelIds.at(channelIndex);
      if (!channelId.isEmpty() && !affectedChannelIdSet.contains(channelId)) {
        affectedChannelIdSet.insert(channelId);
        affectedChannelIds.append(channelId);
      }
    }
  }

  bool changed = false;
  for (int i = 0; i < affectedChannelIds.size(); ++i) {
    QString channelId = affectedChannelIds.at(i);
    int allIndex = m_allDmChannelIndexById.value(channelId, -1).toInt();
    if (allIndex < 0 || allIndex >= m_allDmChannels.size()) {
      continue;
    }

    QVariantMap channel = m_allDmChannels.at(allIndex).toMap();
    QVariantList recipientIds = channel.value("recipientIds").toList();

    // We need to calculate status here since we don't have direct access to
    // ItemMapper's private method
    QString status = "offline";
    int bestRank = 0;
    for (int j = 0; j < recipientIds.size(); ++j) {
      QString uid = recipientIds.at(j).toString().trimmed();
      QString s = m_dmPresenceByUserId.value(uid).toString();
      int rank = 0;
      if (s == "online")
        rank = 3;
      else if (s == "dnd" || s == "busy")
        rank = 2;
      else if (s == "idle")
        rank = 1;

      if (rank > bestRank) {
        bestRank = rank;
        status = s;
      }
    }

    QString statusColor = "#80848E";
    if (status == "online")
      statusColor = "#23A55A";
    else if (status == "idle")
      statusColor = "#F0B232";
    else if (status == "dnd" || status == "busy")
      statusColor = "#F23F43";

    if (channel.value("status").toString() == status &&
        channel.value("statusColor").toString() == statusColor) {
      continue;
    }

    channel["status"] = status;
    channel["statusColor"] = statusColor;
    m_allDmChannels.replace(allIndex, channel);

    int visibleIndex = m_visibleDmChannelIndexById.value(channelId, -1).toInt();
    if (visibleIndex >= 0 && visibleIndex < m_dmChannels.size()) {
      QVariantMap visibleChannel = m_dmChannels.at(visibleIndex).toMap();
      visibleChannel["status"] = status;
      visibleChannel["statusColor"] = statusColor;
      m_dmChannels.replace(visibleIndex, visibleChannel);
      m_dmChannelsById.insert(channelId, visibleChannel);
      changed = true;
    } else if (m_dmChannelsById.contains(channelId)) {
      QVariantMap visibleChannel = m_dmChannelsById.value(channelId).toMap();
      if (!visibleChannel.isEmpty()) {
        visibleChannel["status"] = status;
        visibleChannel["statusColor"] = statusColor;
        m_dmChannelsById.insert(channelId, visibleChannel);
      }
    }
  }

  return changed;
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
      rebuildDmChannelIndexes();
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
      rebuildDmChannelIndexes();
      return;
    }
  }
}

void DiscordClient::updateStoreWithGuildsAndDms() {
  if (m_store) {
    m_store->setGuilds(m_guilds);
    m_store->setDmChannels(m_dmChannels);
  }
}

void DiscordClient::flushGatewayUiUpdates() {
  m_gatewayUiUpdateQueued = false;

  QStringList guildIds = m_pendingUnreadGuildIds;
  QStringList channelIds = m_pendingUnreadChannelIds;
  QVariantMap mentionCounts = m_pendingMentionCountsByGuildId;
  bool dmChanged = m_pendingDmUiUpdate;
  m_pendingUnreadGuildIds.clear();
  m_pendingUnreadChannelIds.clear();
  m_pendingMentionCountsByGuildId.clear();
  m_pendingDmUiUpdate = false;

  if (applyPendingDmPresences()) {
    dmChanged = true;
  }

  for (int i = 0; i < guildIds.size(); ++i) {
    updateGuildUnread(guildIds.at(i), true);
    if (mentionCounts.contains(guildIds.at(i))) {
      updateGuildMentionCount(guildIds.at(i),
                              mentionCounts.value(guildIds.at(i)).toInt());
    }
  }
  for (int i = 0; i < channelIds.size(); ++i) {
    updateGuildChannelUnread(channelIds.at(i), true);
  }

  if (!guildIds.isEmpty()) {
    sortGuilds();
  }

  if (m_store) {
    if (!guildIds.isEmpty()) {
      m_store->setGuilds(m_guilds);
    }
    if (dmChanged) {
      m_store->setDmChannels(m_dmChannels);
    }
    if (!channelIds.isEmpty()) {
      m_store->setGuildChannels(m_visibleGuildChannels);
    }
  }

  if (!guildIds.isEmpty()) {
    saveGuildsCache();
  }
  if (dmChanged) {
    saveDmChannelsCache();
  }
}

void DiscordClient::appendVisibleGuildChannels() {
  int nextCount = m_visibleGuildChannelCount + kPageSize;
  if (nextCount > m_allGuildChannels.size()) {
    nextCount = m_allGuildChannels.size();
  }

  QVariantList appendedChannels;
  for (int i = m_visibleGuildChannelCount; i < nextCount; ++i) {
    m_visibleGuildChannels.append(m_allGuildChannels.at(i));
    appendedChannels.append(m_allGuildChannels.at(i));
  }

  m_visibleGuildChannelCount = nextCount;
  m_guildChannelsHasMore =
      m_visibleGuildChannelCount < m_allGuildChannels.size();
  if (m_store) {
    m_store->appendGuildChannels(appendedChannels);
  }
}

void DiscordClient::updateDataLoading() {
  if (m_store) {
    m_store->setDataLoading(m_loadingGuilds || m_loadingDmChannels ||
                            m_loadingGuildChannels);
  }
}
