#include "Gateway.hpp"

#include "JsonParser.hpp"

#include <QByteArray>
#include <QDebug>
#include <QVariantList>

extern "C" {
#include "mongoose.h"
}

#include <string.h>

namespace {
const char *kGatewayUrl = "wss://gateway.discord.gg/?v=10&encoding=json";
const char *kGatewayHost = "gateway.discord.gg";

QString gatewayEventName(int event) {
  switch (event) {
  case MG_EV_OPEN:
    return "MG_EV_OPEN";
  case MG_EV_CONNECT:
    return "MG_EV_CONNECT";
  case MG_EV_TLS_HS:
    return "MG_EV_TLS_HS";
  case MG_EV_WS_OPEN:
    return "MG_EV_WS_OPEN";
  case MG_EV_WS_MSG:
    return "MG_EV_WS_MSG";
  case MG_EV_CLOSE:
    return "MG_EV_CLOSE";
  case MG_EV_ERROR:
    return "MG_EV_ERROR";
  default:
    return QString("MG_EV_%1").arg(event);
  }
}

} // namespace

DiscordGateway::DiscordGateway(QObject *parent)
    : QObject(parent), m_connection(NULL), m_timerId(0), m_sequence(-1),
      m_heartbeatIntervalMs(0), m_nextHeartbeatMs(0), m_state(Disconnected) {
  m_mgr = new mg_mgr;
  mg_mgr_init(m_mgr);
  mg_log_set(MG_LL_NONE);
}

DiscordGateway::~DiscordGateway() {
  disconnectFromGateway();
  mg_mgr_free(m_mgr);
  delete m_mgr;
}

void DiscordGateway::connectToGateway(const QString &token) {
  disconnectFromGateway();
  resetSession();

  m_token = token.trimmed();
  if (m_token.isEmpty()) {
    emit error("Discord token is empty");
    return;
  }

  setState(Connecting);

  QByteArray headers = QByteArray("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                                  "Win64; x64) AppleWebKit/537.36 (KHTML, like "
                                  "Gecko) Chrome/149.0.0.0 Safari/537.36\r\n") +
                       QByteArray("Origin: https://discord.com\r\n");

  m_connection = mg_ws_connect(m_mgr, kGatewayUrl, DiscordGateway::eventHandler,
                               this, headers.constData());

  if (m_connection == NULL) {
    setState(Disconnected);
    emit error("Could not create Discord gateway connection");
    return;
  }

  if (m_timerId == 0) {
    m_timerId = startTimer(50);
  }
}

void DiscordGateway::disconnectFromGateway() {
  if (m_timerId != 0) {
    killTimer(m_timerId);
    m_timerId = 0;
  }

  if (m_connection != NULL && !m_connection->is_closing) {
    if (m_connection->is_websocket) {
      mg_ws_send(m_connection, "", 0, WEBSOCKET_OP_CLOSE);
    }

    m_connection->is_closing = 1;

    for (int i = 0; i < 10 && m_connection != NULL; ++i) {
      mg_mgr_poll(m_mgr, 50);
    }
  }

  m_connection = NULL;
  setState(Disconnected);
}

DiscordGateway::ConnectionState DiscordGateway::state() const {
  return m_state;
}

void DiscordGateway::timerEvent(QTimerEvent *event) {
  Q_UNUSED(event);

  mg_mgr_poll(m_mgr, 0);

  if (m_connection != NULL && m_connection->is_websocket &&
      m_heartbeatIntervalMs > 0 && mg_millis() >= m_nextHeartbeatMs) {
    sendHeartbeat();
  }
}

void DiscordGateway::sendHeartbeat() {
  QString sequence = m_sequence >= 0 ? QString::number(m_sequence) : "null";
  sendJsonText(QString("{\"op\":1,\"d\":%1}").arg(sequence));
  m_nextHeartbeatMs = mg_millis() + m_heartbeatIntervalMs;
  qDebug() << "[discord] heartbeat sent" << sequence;
}

void DiscordGateway::eventHandler(struct mg_connection *connection, int event,
                                  void *eventData) {
  DiscordGateway *gateway = static_cast<DiscordGateway *>(connection->fn_data);
  if (gateway != NULL) {
    gateway->handleEvent(connection, event, eventData);
  }
}

void DiscordGateway::handleEvent(struct mg_connection *connection, int event,
                                 void *eventData) {
  if (event != MG_EV_POLL && event != MG_EV_READ && event != MG_EV_WRITE) {
    qDebug() << "[discord-gateway]" << gatewayEventName(event)
             << "fd=" << connection->fd << "tls=" << connection->is_tls
             << "ws=" << connection->is_websocket
             << "closing=" << connection->is_closing;
  }

  switch (event) {
  case MG_EV_CONNECT:
    if (connection->is_tls) {
      struct mg_tls_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.name = mg_str(kGatewayHost);
      opts.skip_verification = true;
      mg_tls_init(connection, &opts);
    }
    break;

  case MG_EV_RESOLVE:
    qDebug() << "[discord-gateway] resolved host";
    break;

  case MG_EV_WS_OPEN:
    setState(Connected);
    break;

  case MG_EV_WS_MSG: {
    struct mg_ws_message *message =
        static_cast<struct mg_ws_message *>(eventData);
    handleTextMessage(message->data.buf, static_cast<int>(message->data.len));
    break;
  }

  case MG_EV_ERROR: {
    QString message = QString::fromUtf8(
        eventData ? static_cast<char *>(eventData) : "Gateway error");
    if (message == "MG_MAX_RECV_SIZE" && m_state == Connected) {
      qDebug() << "[discord] gateway READY is too large; accepting login and "
                  "closing gateway";
      setState(Ready);
      emit ready(QString());
      break;
    }

    emit error(message);
    break;
  }

  case MG_EV_CLOSE:
    if (m_connection == connection) {
      m_connection = NULL;
    }
    if (m_timerId != 0) {
      killTimer(m_timerId);
      m_timerId = 0;
    }
    setState(Disconnected);
    emit closed();
    break;
  }
}

void DiscordGateway::handleTextMessage(const char *data, int length) {
  QByteArray bytes(data, length);
  qDebug() << "[discord] websocket message bytes" << length;

  if (DiscordJsonParser::isLargeReadyPayload(bytes)) {
    m_sequence = DiscordJsonParser::valueToInt(
        DiscordJsonParser::extractStringField(bytes, "s"), m_sequence);
    m_sessionId = DiscordJsonParser::extractStringField(bytes, "session_id");
    m_resumeGatewayUrl =
        DiscordJsonParser::extractStringField(bytes, "resume_gateway_url");

    qDebug() << "[discord] large READY received; skipping full JSON parse"
             << "session" << m_sessionId;
    setState(Ready);
    emit ready(m_sessionId);
    return;
  }

  DiscordJsonParser::GatewayPayload payload =
      DiscordJsonParser::parseGatewayPayload(bytes);
  if (!payload.valid) {
    emit error(
        QString("Gateway JSON parse error: %1").arg(payload.errorMessage));
    return;
  }

  qDebug() << "[discord] gateway payload op" << payload.op << "event"
           << payload.eventName << "sequence" << payload.sequence;

  if (payload.sequence >= 0) {
    m_sequence = payload.sequence;
  }

  switch (payload.op) {
  case 0:
    handleDispatch(payload.eventName, payload.data, payload.sequence);
    break;
  case 10:
    handleHello(payload.data);
    break;
  case 11:
    qDebug() << "[discord] heartbeat ACK";
    if (m_state == Connected) {
      qDebug() << "[discord] heartbeat ACK received; accepting login before "
                  "large READY payload";
      setState(Ready);
      emit ready(QString());
      if (m_connection != NULL) {
        m_connection->is_closing = 1;
      }
    }
    break;
  case 7:
    emit error("Discord gateway requested reconnect");
    break;
  case 9:
    emit error("Discord gateway invalid session");
    break;
  default:
    qDebug() << "[discord] unhandled gateway opcode" << payload.op;
    break;
  }
}

void DiscordGateway::handleHello(const QVariantMap &data) {
  int interval =
      DiscordJsonParser::valueToInt(data.value("heartbeat_interval"), 0);
  if (interval <= 0) {
    emit error("Discord gateway HELLO did not include heartbeat interval");
    return;
  }

  m_heartbeatIntervalMs = interval;
  m_nextHeartbeatMs = mg_millis() + m_heartbeatIntervalMs;
  sendHeartbeat();
  sendIdentify();
}

void DiscordGateway::handleDispatch(const QString &eventName,
                                    const QVariantMap &data, int sequence) {
  Q_UNUSED(sequence);

  if (eventName == "READY") {
    m_sessionId = data.value("session_id").toString();
    m_resumeGatewayUrl = data.value("resume_gateway_url").toString();
    setState(Ready);
    emit ready(m_sessionId);
  }

  emit dispatchReceived(eventName, data);
}

void DiscordGateway::sendIdentify() {
  QString errorMessage;
  QByteArray payload =
      DiscordJsonParser::buildIdentifyPayload(m_token, &errorMessage);
  if (!errorMessage.isEmpty()) {
    emit error(QString("Gateway identify JSON error: %1").arg(errorMessage));
    return;
  }

  sendJsonText(QString::fromUtf8(payload.constData(), payload.size()));
}

void DiscordGateway::sendJsonText(const QString &text) {
  if (m_connection == NULL || !m_connection->is_websocket ||
      m_connection->is_closing) {
    return;
  }

  QByteArray payload = text.toUtf8();
  mg_ws_send(m_connection, payload.constData(), payload.size(),
             WEBSOCKET_OP_TEXT);
}

void DiscordGateway::setState(ConnectionState state) {
  if (m_state == state) {
    return;
  }

  m_state = state;
  emit stateChanged(static_cast<int>(m_state));
}

void DiscordGateway::resetSession() {
  m_sequence = -1;
  m_heartbeatIntervalMs = 0;
  m_nextHeartbeatMs = 0;
  m_sessionId.clear();
  m_resumeGatewayUrl.clear();
}
