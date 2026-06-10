#include "Worker.hpp"

#include "../AppStore.hpp"
#include "../AvatarCacheWorker.hpp"
#include "../Client.hpp"
#include "../client/AvatarManager.hpp"
#include "../client/CacheManager.hpp"
#include "../client/GatewayHandler.hpp"
#include "../client/ItemMapper.hpp"
#include "../client/SortUtils.hpp"
#include "../discord/GatewayWorker.hpp"
#include "../discord/NetworkWorker.hpp"

#include <QMetaObject>
#include <QMetaType>
#include <QThread>

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
  qRegisterMetaType<QStringList>("QStringList");

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
  connect(m_networkWorker,
          SIGNAL(channelMessagesLoaded(QString, QString, QVariantList)), this,
          SLOT(onChannelMessagesLoaded(QString, QString, QVariantList)),
          Qt::QueuedConnection);
  connect(m_networkWorker,
          SIGNAL(channelMessageSent(QString, QString, QVariantMap)), this,
          SLOT(onChannelMessageSent(QString, QString, QVariantMap)),
          Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(channelMessageEdited(QString, QVariantMap)),
          this, SLOT(onChannelMessageEdited(QString, QVariantMap)),
          Qt::QueuedConnection);
  connect(m_networkWorker, SIGNAL(channelMessageDeleted(QString, QString)),
          this, SLOT(onChannelMessageDeleted(QString, QString)),
          Qt::QueuedConnection);
  connect(m_networkWorker,
          SIGNAL(chatRequestFailed(QString, QString, QString, QString)), this,
          SLOT(onChatRequestFailed(QString, QString, QString, QString)),
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
  connect(m_networkWorker, SIGNAL(cancelAllFinished()), m_networkThread,
          SLOT(quit()), Qt::DirectConnection);

  m_networkThread->start();
}

void DiscordClient::initializeGatewayWorker() {
  qRegisterMetaType<QVariantMap>("QVariantMap");
  qRegisterMetaType<QVariantList>("QVariantList");
  qRegisterMetaType<QStringList>("QStringList");

  if (m_gatewayThread != 0 || m_gatewayWorker != 0) {
    return;
  }

  m_gatewayThread = new QThread(this);
  m_gatewayWorker = new DiscordGatewayWorker();
  m_gatewayWorker->moveToThread(m_gatewayThread);

  connect(m_gatewayThread, SIGNAL(finished()), m_gatewayWorker,
          SLOT(deleteLater()));
  connect(m_gatewayWorker,
          SIGNAL(gatewayDispatchReceived(QString, QVariantMap)), this,
          SLOT(onGatewayDispatch(QString, QVariantMap)), Qt::QueuedConnection);
  connect(m_gatewayWorker, SIGNAL(gatewayReady(QString)), this,
          SLOT(onGatewayReady(QString)), Qt::QueuedConnection);
  connect(m_gatewayWorker, SIGNAL(gatewayError(QString)), this,
          SLOT(onGatewayError(QString)), Qt::QueuedConnection);
  connect(m_gatewayWorker, SIGNAL(gatewayClosed()), this,
          SLOT(onGatewayClosed()), Qt::QueuedConnection);
  connect(
      m_gatewayWorker,
      SIGNAL(guildsAndDmsReady(QVariantList, QVariantList, QVariantList,
                               QStringList, QVariantMap)),
      this,
      SLOT(onGatewayGuildsAndDmsReady(QVariantList, QVariantList, QVariantList,
                                      QStringList, QVariantMap)),
      Qt::QueuedConnection);
  connect(m_gatewayWorker, SIGNAL(cancelAllFinished()), m_gatewayThread,
          SLOT(quit()), Qt::DirectConnection);

  m_gatewayThread->start();
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

void DiscordClient::shutdownGatewayWorker() {
  if (m_gatewayWorker != 0 && m_gatewayThread != 0 &&
      m_gatewayThread->isRunning()) {
    QMetaObject::invokeMethod(m_gatewayWorker, "cancelAll",
                              Qt::QueuedConnection);
    m_gatewayThread->wait();
  } else if (m_gatewayWorker != 0) {
    delete m_gatewayWorker;
  }

  m_gatewayWorker = 0;

  if (m_gatewayThread != 0) {
    if (!m_gatewayThread->isFinished()) {
      m_gatewayThread->quit();
      m_gatewayThread->wait();
    }
    delete m_gatewayThread;
    m_gatewayThread = 0;
  }
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

void DiscordClient::syncStateToNetworkWorker() {
  syncGatewayOrderingStateToWorker();
}

void DiscordClient::syncGatewayOrderingStateToWorker() {
  if (m_gatewayWorker == 0) {
    return;
  }

  m_gatewayWorker->updateGatewayOrderingState(m_guilds, m_allDmChannels,
                                              m_dmChannels, m_orderedGuildIds,
                                              m_dmPresenceByUserId);
}

void DiscordClient::syncGatewayMessageFilterStateToWorker() {
  if (m_gatewayWorker == 0) {
    return;
  }

  QString selectedChannelId =
      m_store ? m_store->selectedChannelId() : QString();
  QStringList loadedChannelIds =
      m_store ? m_store->loadedChatChannelIds() : QStringList();
  QString currentUserId = m_store ? m_store->currentUserId() : QString();
  m_gatewayWorker->updateGatewayMessageFilterState(
      selectedChannelId, loadedChannelIds, currentUserId);
}
