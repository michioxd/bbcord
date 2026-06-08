#include "Gateway.hpp"

#include "DiscordUtils.hpp"
#include "JsonParser.hpp"

#include <QByteArray>
#include <QDebug>

extern "C" {
#include "mongoose.h"
}

#include <string.h>

namespace {
const char *kGatewayUrl =
    "wss://gateway.discord.gg/?v=10&encoding=json&compress=zlib-stream";
const char *kGatewayHost = "gateway.discord.gg";
} // namespace

DiscordGateway::DiscordGateway(QObject *parent)
    : QObject(parent), m_connection(NULL), m_timerId(0), m_sequence(-1),
      m_zstreamReady(false), m_heartbeatIntervalMs(0), m_nextHeartbeatMs(0),
      m_state(Disconnected) {
  m_mgr = new mg_mgr;
  mg_mgr_init(m_mgr);
  mg_log_set(MG_LL_NONE);
  memset(&m_zstream, 0, sizeof(m_zstream));
  m_zstreamReady = inflateInit(&m_zstream) == Z_OK;
}

DiscordGateway::~DiscordGateway() {
  disconnectFromGateway();
  if (m_zstreamReady) {
    inflateEnd(&m_zstream);
    m_zstreamReady = false;
  }
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

  QByteArray headers = DiscordUtils::desktopUserAgentHeader() +
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

void DiscordGateway::initializeTls(struct mg_connection *connection) {
  if (connection != NULL && connection->is_tls) {
    struct mg_tls_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.name = mg_str(kGatewayHost);
    opts.skip_verification = true;
    mg_tls_init(connection, &opts);
  }
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
  m_compressedBuffer.clear();
  if (m_zstreamReady) {
    inflateReset(&m_zstream);
  }
}
