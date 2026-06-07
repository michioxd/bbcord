#include "Client.hpp"

#include "AppStore.hpp"
#include "models/Models.hpp"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

DiscordClient::DiscordClient(QObject *parent)
    : QObject(parent), m_store(0), m_restClient(this), m_loggedIn(false),
      m_busy(false), m_statusText("Disconnected") {
  connect(&m_restClient, SIGNAL(loginSucceeded(QVariantMap)), this,
          SLOT(onRestLoginSucceeded(QVariantMap)));
  connect(&m_restClient, SIGNAL(loginFailed(QString)), this,
          SLOT(onRestLoginFailed(QString)));
  connect(&m_restClient, SIGNAL(avatarDownloaded(QString)), this,
          SLOT(onAvatarDownloaded(QString)));
  connect(&m_restClient, SIGNAL(avatarDownloadFailed(QString)), this,
          SLOT(onAvatarDownloadFailed(QString)));

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::DiscordClient(AppStore *store, QObject *parent)
    : QObject(parent), m_store(store), m_restClient(this), m_loggedIn(false),
      m_busy(false), m_statusText("Disconnected") {
  connect(&m_restClient, SIGNAL(loginSucceeded(QVariantMap)), this,
          SLOT(onRestLoginSucceeded(QVariantMap)));
  connect(&m_restClient, SIGNAL(loginFailed(QString)), this,
          SLOT(onRestLoginFailed(QString)));
  connect(&m_restClient, SIGNAL(avatarDownloaded(QString)), this,
          SLOT(onAvatarDownloaded(QString)));
  connect(&m_restClient, SIGNAL(avatarDownloadFailed(QString)), this,
          SLOT(onAvatarDownloadFailed(QString)));

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::~DiscordClient() { m_restClient.cancel(); }

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
  emit loginSucceeded();
}

void DiscordClient::onRestLoginFailed(const QString &message) {
  qDebug() << "[discord-client] REST login failed" << message;
  setBusy(false);
  setLoggedIn(false);
  setStatusText(message);
  emit loginFailed(message);
}

void DiscordClient::onAvatarDownloaded(const QString &localPath) {
  if (m_store) {
    m_store->setCurrentUserAvatarSource(avatarSourceForPath(localPath));
  }
}

void DiscordClient::onAvatarDownloadFailed(const QString &message) {
  qDebug() << "[discord-client] avatar download failed" << message;
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

  m_restClient.downloadAvatar(user.id, user.avatarHash, path);
}

QString DiscordClient::avatarCachePath(const DiscordUser &user) const {
  QDir dir(QDir::homePath());
  dir.mkpath("data/avatar-cache");
  return dir.absoluteFilePath(
      QString("data/avatar-cache/%1_%2.png").arg(user.id).arg(user.avatarHash));
}

QString DiscordClient::avatarSourceForPath(const QString &path) const {
  QString cleanPath = QDir::fromNativeSeparators(path);
  if (cleanPath.startsWith("/")) {
    return QString("file://%1").arg(cleanPath);
  }

  return QString("file:///%1").arg(cleanPath);
}
