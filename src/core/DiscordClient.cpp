#include "DiscordClient.hpp"

#include <QDebug>
#include <QSettings>

DiscordClient::DiscordClient(QObject *parent)
    : QObject(parent), m_gateway(this), m_loggedIn(false), m_busy(false),
      m_statusText("Disconnected") {
  connect(&m_gateway, SIGNAL(ready(QString)), this,
          SLOT(onGatewayReady(QString)));
  connect(&m_gateway, SIGNAL(error(QString)), this,
          SLOT(onGatewayError(QString)));
  connect(&m_gateway, SIGNAL(closed()), this, SLOT(onGatewayClosed()));
  connect(&m_gateway, SIGNAL(stateChanged(int)), this,
          SLOT(onGatewayStateChanged(int)));

  QSettings settings;
  m_token = settings.value("auth/token").toString();
}

DiscordClient::~DiscordClient() { m_gateway.disconnectFromGateway(); }

void DiscordClient::login(const QString &token) {
  QString trimmedToken = token.trimmed();
  if (trimmedToken.isEmpty()) {
    setStatusText("Token is empty");
    emit loginFailed(m_statusText);
    return;
  }

  setLoggedIn(false);
  setBusy(true);
  setStatusText("Connecting to Discord gateway...");

  m_token = trimmedToken;
  m_gateway.connectToGateway(trimmedToken);
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
  m_gateway.disconnectFromGateway();
  setBusy(false);
  setLoggedIn(false);
  setStatusText("Disconnected");
}

bool DiscordClient::loggedIn() const { return m_loggedIn; }

bool DiscordClient::busy() const { return m_busy; }

QString DiscordClient::statusText() const { return m_statusText; }

void DiscordClient::onGatewayReady(const QString &sessionId) {
  qDebug() << "[discord-client] gateway ready" << sessionId;
  saveToken();
  setBusy(false);
  setLoggedIn(true);
  setStatusText("Connected");
  emit loginSucceeded();
}

void DiscordClient::onGatewayError(const QString &message) {
  qDebug() << "[discord-client] gateway error" << message;
  setBusy(false);
  setLoggedIn(false);
  setStatusText(message);
  emit loginFailed(message);
}

void DiscordClient::onGatewayClosed() {
  if (m_loggedIn) {
    setBusy(false);
    setStatusText("Connected");
    return;
  }

  setBusy(false);
  setLoggedIn(false);
  setStatusText("Disconnected");
}

void DiscordClient::onGatewayStateChanged(int state) {
  switch (state) {
  case DiscordGateway::Connecting:
    setStatusText("Connecting...");
    break;
  case DiscordGateway::Connected:
    setStatusText("Authenticating...");
    break;
  case DiscordGateway::Ready:
    setStatusText("Connected");
    break;
  case DiscordGateway::Disconnected:
  default:
    if (m_loggedIn) {
      setStatusText("Connected");
      break;
    }
    if (!m_busy) {
      setStatusText("Disconnected");
    }
    break;
  }
}

void DiscordClient::setLoggedIn(bool loggedIn) {
  if (m_loggedIn == loggedIn) {
    return;
  }

  m_loggedIn = loggedIn;
  emit loggedInChanged(m_loggedIn);
}

void DiscordClient::setBusy(bool busy) {
  if (m_busy == busy) {
    return;
  }

  m_busy = busy;
  emit busyChanged(m_busy);
}

void DiscordClient::setStatusText(const QString &statusText) {
  if (m_statusText == statusText) {
    return;
  }

  m_statusText = statusText;
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
