#include "DiscordGateway.hpp"

#include <bb/data/JsonDataAccess>

#include <QByteArray>
#include <QDebug>
#include <QVariant>
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

int variantToInt(const QVariant &value, int fallback) {
  bool ok = false;
  int result = value.toInt(&ok);
  return ok ? result : fallback;
}

bool hasJsonToken(const QByteArray &bytes, const char *compactToken,
                  const char *spacedToken) {
  return bytes.indexOf(compactToken) >= 0 || bytes.indexOf(spacedToken) >= 0;
}

QString extractJsonString(const QByteArray &bytes, const char *fieldName) {
  QByteArray marker = QByteArray("\"") + fieldName + QByteArray("\"");
  int pos = bytes.indexOf(marker);
  if (pos < 0) {
    return QString();
  }

  pos = bytes.indexOf(':', pos + marker.size());
  if (pos < 0) {
    return QString();
  }

  ++pos;
  while (pos < bytes.size() &&
         (bytes.at(pos) == ' ' || bytes.at(pos) == '\t' ||
          bytes.at(pos) == '\r' || bytes.at(pos) == '\n')) {
    ++pos;
  }

  if (pos >= bytes.size() || bytes.at(pos) != '"') {
    return QString();
  }

  ++pos;
  QByteArray value;
  bool escaped = false;
  for (; pos < bytes.size(); ++pos) {
    char ch = bytes.at(pos);
    if (escaped) {
      value.append(ch);
      escaped = false;
    } else if (ch == '\\') {
      escaped = true;
    } else if (ch == '"') {
      break;
    } else {
      value.append(ch);
    }
  }

  return QString::fromUtf8(value.constData(), value.size());
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

  if (length > 262144 && hasJsonToken(bytes, "\"op\":0", "\"op\": 0") &&
      hasJsonToken(bytes, "\"t\":\"READY\"", "\"t\": \"READY\"")) {
    m_sequence = variantToInt(extractJsonString(bytes, "s"), m_sequence);
    m_sessionId = extractJsonString(bytes, "session_id");
    m_resumeGatewayUrl = extractJsonString(bytes, "resume_gateway_url");

    qDebug() << "[discord] large READY received; skipping full JSON parse"
             << "session" << m_sessionId;
    setState(Ready);
    emit ready(m_sessionId);
    return;
  }

  bb::data::JsonDataAccess json;
  QVariant parsed = json.loadFromBuffer(bytes);

  if (json.hasError()) {
    emit error(QString("Gateway JSON parse error: %1")
                   .arg(json.error().errorMessage()));
    return;
  }

  QVariantMap root = parsed.toMap();
  int op = variantToInt(root.value("op"), -1);
  int sequence = variantToInt(root.value("s"), -1);
  QString eventName = root.value("t").toString();
  QVariantMap eventData = root.value("d").toMap();

  qDebug() << "[discord] gateway payload op" << op << "event" << eventName
           << "sequence" << sequence;

  if (sequence >= 0) {
    m_sequence = sequence;
  }

  switch (op) {
  case 0:
    handleDispatch(eventName, eventData, sequence);
    break;
  case 10:
    handleHello(eventData);
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
    qDebug() << "[discord] unhandled gateway opcode" << op;
    break;
  }
}

void DiscordGateway::handleHello(const QVariantMap &data) {
  int interval = variantToInt(data.value("heartbeat_interval"), 0);
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
  QVariantMap properties;
  properties["os"] = "BlackBerry 10";
  properties["browser"] = "BBCord";
  properties["device"] = "BBCord";

  QVariantMap data;
  data["token"] = m_token;
  data["intents"] = 0;
  data["large_threshold"] = 50;
  data["properties"] = properties;

  QVariantMap root;
  root["op"] = 2;
  root["d"] = data;

  bb::data::JsonDataAccess json;
  QByteArray payload;
  json.saveToBuffer(root, &payload);
  if (json.hasError()) {
    emit error(QString("Gateway identify JSON error: %1")
                   .arg(json.error().errorMessage()));
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
