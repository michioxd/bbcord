#ifndef RestClient_HPP_
#define RestClient_HPP_

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

struct mg_mgr;
struct mg_connection;

class DiscordRestClient : public QObject {
  Q_OBJECT

public:
  explicit DiscordRestClient(QObject *parent = 0);
  virtual ~DiscordRestClient();

  void loginWithToken(const QString &token);
  void downloadAvatar(const QString &userId, const QString &avatarHash,
                      const QString &outputPath);
  void cancel();

Q_SIGNALS:
  void loginSucceeded(const QVariantMap &user);
  void loginFailed(const QString &message);
  void avatarDownloaded(const QString &localPath);
  void avatarDownloadFailed(const QString &message);

protected:
  virtual void timerEvent(QTimerEvent *event);

private:
  enum RequestType { NoRequest, LoginRequest, AvatarRequest };

  static void eventHandler(struct mg_connection *connection, int event,
                           void *eventData);
  void handleEvent(struct mg_connection *connection, int event,
                   void *eventData);
  void startTimerIfNeeded();
  void stopTimerIfIdle();
  void finishRequest();
  void failWithMessage(const QString &message);
  void succeedWithUser(const QVariantMap &user);
  void sendGetMeRequest(struct mg_connection *connection);
  void sendAvatarRequest(struct mg_connection *connection);

  mg_mgr *m_mgr;
  mg_connection *m_connection;
  int m_timerId;
  int m_pollTicks;
  RequestType m_requestType;
  QString m_token;
  QString m_avatarUserId;
  QString m_avatarHash;
  QString m_outputPath;
  bool m_requestSent;
  bool m_finished;
};

#endif /* RestClient_HPP_ */
