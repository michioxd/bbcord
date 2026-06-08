#include "Gateway.hpp"

#include "JsonParser.hpp"

#include <QByteArray>
#include <QDebug>
#include <QVariantList>

extern "C" {
#include "mongoose.h"
}

namespace {
int extractSequence(const QByteArray &bytes) {
  int pos = bytes.indexOf("\"s\"");
  if (pos < 0) {
    return -1;
  }

  pos = bytes.indexOf(':', pos + 3);
  if (pos < 0) {
    return -1;
  }

  ++pos;
  while (pos < bytes.size() &&
         (bytes.at(pos) == ' ' || bytes.at(pos) == '\t' ||
          bytes.at(pos) == '\r' || bytes.at(pos) == '\n')) {
    ++pos;
  }

  if (pos < bytes.size() && bytes.mid(pos, 4) == "null") {
    return -1;
  }

  bool negative = false;
  if (pos < bytes.size() && bytes.at(pos) == '-') {
    negative = true;
    ++pos;
  }

  int value = 0;
  bool foundDigit = false;
  while (pos < bytes.size() && bytes.at(pos) >= '0' && bytes.at(pos) <= '9') {
    foundDigit = true;
    value = value * 10 + (bytes.at(pos) - '0');
    ++pos;
  }

  if (!foundDigit) {
    return -1;
  }
  return negative ? -value : value;
}

bool shouldParseDispatch(const QString &eventName) {
  return eventName == "MESSAGE_CREATE" || eventName == "MESSAGE_UPDATE" ||
         eventName == "MESSAGE_DELETE" || eventName == "READY" ||
         eventName == "GUILD_CREATE" || eventName == "GUILD_DELETE" ||
         eventName == "USER_SETTINGS_PROTO_UPDATE";
}
} // namespace

void DiscordGateway::eventHandler(struct mg_connection *connection, int event,
                                  void *eventData) {
  DiscordGateway *gateway = static_cast<DiscordGateway *>(connection->fn_data);
  if (gateway != NULL) {
    gateway->handleEvent(connection, event, eventData);
  }
}

void DiscordGateway::handleEvent(struct mg_connection *connection, int event,
                                 void *eventData) {
  if (event != MG_EV_POLL && event != MG_EV_READ && event != MG_EV_WRITE &&
      event != MG_EV_WS_MSG) {
    qDebug() << "[discord-gateway] event" << event << "fd=" << connection->fd
             << "tls=" << connection->is_tls
             << "ws=" << connection->is_websocket
             << "closing=" << connection->is_closing;
  }

  switch (event) {
  case MG_EV_CONNECT:
    initializeTls(connection);
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
    handleCompressedMessage(message->data.buf,
                            static_cast<int>(message->data.len));
    break;
  }

  case MG_EV_ERROR: {
    QString message = QString::fromUtf8(
        eventData ? static_cast<char *>(eventData) : "Gateway error");

    qDebug() << "[discord-gateway] error" << message;
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

void DiscordGateway::handleCompressedMessage(const char *data, int length) {
  if (!m_zstreamReady || data == NULL || length <= 0) {
    handleTextMessage(data, length);
    return;
  }

  m_compressedBuffer.append(data, length);
  if (m_compressedBuffer.size() < 4 ||
      static_cast<unsigned char>(
          m_compressedBuffer.at(m_compressedBuffer.size() - 4)) != 0x00 ||
      static_cast<unsigned char>(
          m_compressedBuffer.at(m_compressedBuffer.size() - 3)) != 0x00 ||
      static_cast<unsigned char>(
          m_compressedBuffer.at(m_compressedBuffer.size() - 2)) != 0xff ||
      static_cast<unsigned char>(
          m_compressedBuffer.at(m_compressedBuffer.size() - 1)) != 0xff) {
    return;
  }

  QByteArray inflated;
  char output[32768];
  m_zstream.next_in = reinterpret_cast<Bytef *>(m_compressedBuffer.data());
  m_zstream.avail_in = static_cast<uInt>(m_compressedBuffer.size());

  int result = Z_OK;
  do {
    m_zstream.next_out = reinterpret_cast<Bytef *>(output);
    m_zstream.avail_out = sizeof(output);
    result = inflate(&m_zstream, Z_SYNC_FLUSH);
    if (result != Z_OK && result != Z_BUF_ERROR) {
      emit error(QString("Gateway zlib inflate error %1").arg(result));
      inflateReset(&m_zstream);
      m_compressedBuffer.clear();
      return;
    }

    int produced = static_cast<int>(sizeof(output) - m_zstream.avail_out);
    if (produced > 0) {
      inflated.append(output, produced);
    }
  } while (m_zstream.avail_out == 0);

  m_compressedBuffer.clear();
  if (!inflated.isEmpty()) {
    handleTextMessage(inflated.constData(), inflated.size());
  }
}

void DiscordGateway::handleTextMessage(const char *data, int length) {
  QByteArray bytes(data, length);

  if (DiscordJsonParser::isLargeReadyPayload(bytes)) {
    m_sequence = DiscordJsonParser::valueToInt(
        DiscordJsonParser::extractStringField(bytes, "s"), m_sequence);
    m_sessionId = DiscordJsonParser::extractStringField(bytes, "session_id");
    m_resumeGatewayUrl =
        DiscordJsonParser::extractStringField(bytes, "resume_gateway_url");
    QString userSettingsProto =
        DiscordJsonParser::extractStringField(bytes, "user_settings_proto");
    QVariantList presences =
        DiscordJsonParser::extractArrayField(bytes, "presences");

    qDebug() << "[discord] large READY received; skipping full JSON parse"
             << "session" << m_sessionId << "presences" << presences.size();
    setState(Ready);
    emit ready(m_sessionId);
    QVariantMap readyPayload;
    readyPayload["session_id"] = m_sessionId;
    readyPayload["resume_gateway_url"] = m_resumeGatewayUrl;
    readyPayload["user_settings_proto"] = userSettingsProto;
    readyPayload["presences"] = presences;
    emit dispatchReceived("READY", readyPayload);
    return;
  }

  QString fastEventName = DiscordJsonParser::extractStringField(bytes, "t");
  if (!fastEventName.isEmpty() && !shouldParseDispatch(fastEventName)) {
    int sequence = extractSequence(bytes);
    if (sequence >= 0) {
      m_sequence = sequence;
    }
    return;
  }

  DiscordJsonParser::GatewayPayload payload =
      DiscordJsonParser::parseGatewayPayload(bytes);
  if (!payload.valid) {
    emit error(
        QString("Gateway JSON parse error: %1").arg(payload.errorMessage));
    return;
  }
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
