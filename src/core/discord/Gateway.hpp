#ifndef Gateway_HPP_
#define Gateway_HPP_

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <stdint.h>

struct mg_mgr;
struct mg_connection;
class QTimerEvent;

class DiscordGateway : public QObject {
  Q_OBJECT

public:
  enum ConnectionState { Disconnected, Connecting, Connected, Ready };

  explicit DiscordGateway(QObject *parent = 0);
  virtual ~DiscordGateway();

  Q_INVOKABLE void connectToGateway(const QString &token);
  Q_INVOKABLE void disconnectFromGateway();

  ConnectionState state() const;

Q_SIGNALS:
  void stateChanged(int state);
  void ready(const QString &sessionId);
  void dispatchReceived(const QString &eventName, const QVariantMap &payload);
  void error(const QString &message);
  void closed();

protected:
  void timerEvent(QTimerEvent *event);

private:
  static void eventHandler(struct mg_connection *connection, int event,
                           void *eventData);

  void handleEvent(struct mg_connection *connection, int event,
                   void *eventData);
  void handleTextMessage(const char *data, int length);
  void handleHello(const QVariantMap &data);
  void handleDispatch(const QString &eventName, const QVariantMap &data,
                      int sequence);
  void sendHeartbeat();
  void sendIdentify();
  void sendJsonText(const QString &text);
  void setState(ConnectionState state);
  void resetSession();

  mg_mgr *m_mgr;
  mg_connection *m_connection;
  int m_timerId;
  QString m_token;
  QString m_sessionId;
  QString m_resumeGatewayUrl;
  int m_sequence;
  int m_heartbeatIntervalMs;
  uint64_t m_nextHeartbeatMs;
  ConnectionState m_state;
};

#endif /* Gateway_HPP_ */
