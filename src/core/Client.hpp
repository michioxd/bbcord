#ifndef Client_HPP_
#define Client_HPP_

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "discord/RestClient.hpp"
#include "models/Models.hpp"

class AppStore;

class DiscordClient : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
  explicit DiscordClient(QObject *parent = 0);
  explicit DiscordClient(AppStore *store, QObject *parent = 0);
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
  void onRestLoginSucceeded(const QVariantMap &user);
  void onRestLoginFailed(const QString &message);
  void onAvatarDownloaded(const QString &localPath);
  void onAvatarDownloadFailed(const QString &message);

private:
  void setLoggedIn(bool loggedIn);
  void setBusy(bool busy);
  void setStatusText(const QString &statusText);
  void saveToken();
  void clearSavedToken();
  void loadCurrentUserAvatar(const DiscordUser &user);
  QString avatarCachePath(const DiscordUser &user) const;
  QString avatarSourceForPath(const QString &path) const;

  AppStore *m_store;
  DiscordRestClient m_restClient;
  QString m_token;
  bool m_loggedIn;
  bool m_busy;
  QString m_statusText;
};

#endif /* Client_HPP_ */
