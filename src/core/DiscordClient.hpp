#ifndef DiscordClient_HPP_
#define DiscordClient_HPP_

#include <QObject>
#include <QString>

#include "discord/DiscordGateway.hpp"

class DiscordClient : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
  explicit DiscordClient(QObject *parent = 0);
  virtual ~DiscordClient();

  Q_INVOKABLE void login(const QString &token);
  Q_INVOKABLE void autoLogin();
  Q_INVOKABLE void logout();

  bool loggedIn() const;
  bool busy() const;
  QString statusText() const;

Q_SIGNALS:
  void loginSucceeded();
  void loginFailed(const QString &message);
  void loggedInChanged(bool loggedIn);
  void busyChanged(bool busy);
  void statusTextChanged(const QString &statusText);

private Q_SLOTS:
  void onGatewayReady(const QString &sessionId);
  void onGatewayError(const QString &message);
  void onGatewayClosed();
  void onGatewayStateChanged(int state);

private:
  void setLoggedIn(bool loggedIn);
  void setBusy(bool busy);
  void setStatusText(const QString &statusText);
  void saveToken();
  void clearSavedToken();

  DiscordGateway m_gateway;
  QString m_token;
  bool m_loggedIn;
  bool m_busy;
  QString m_statusText;
};

#endif /* DiscordClient_HPP_ */
