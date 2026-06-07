#include "JsonParser.hpp"

#include <bb/data/JsonDataAccess>

DiscordJsonParser::GatewayPayload
DiscordJsonParser::parseGatewayPayload(const QByteArray &bytes) {
  GatewayPayload payload;

  bb::data::JsonDataAccess json;
  QVariant parsed = json.loadFromBuffer(bytes);

  if (json.hasError()) {
    payload.errorMessage = json.error().errorMessage();
    return payload;
  }

  QVariantMap root = parsed.toMap();
  payload.op = valueToInt(root.value("op"), -1);
  payload.sequence = valueToInt(root.value("s"), -1);
  payload.eventName = root.value("t").toString();
  payload.data = root.value("d").toMap();
  payload.valid = true;

  return payload;
}

QVariantMap DiscordJsonParser::parseObject(const QByteArray &bytes,
                                           QString *errorMessage) {
  bb::data::JsonDataAccess json;
  QVariant parsed = json.loadFromBuffer(bytes);

  if (json.hasError()) {
    if (errorMessage != 0) {
      *errorMessage = json.error().errorMessage();
    }
    return QVariantMap();
  }

  if (errorMessage != 0) {
    errorMessage->clear();
  }
  return parsed.toMap();
}

QVariantList DiscordJsonParser::parseArray(const QByteArray &bytes,
                                           QString *errorMessage) {
  bb::data::JsonDataAccess json;
  QVariant parsed = json.loadFromBuffer(bytes);

  if (json.hasError()) {
    if (errorMessage != 0) {
      *errorMessage = json.error().errorMessage();
    }
    return QVariantList();
  }

  if (errorMessage != 0) {
    errorMessage->clear();
  }
  return parsed.toList();
}

QByteArray DiscordJsonParser::buildIdentifyPayload(const QString &token,
                                                   QString *errorMessage) {
  QVariantMap properties;
  properties["os"] = "BlackBerry 10";
  properties["browser"] = "BBCord";
  properties["device"] = "BBCord";

  QVariantMap data;
  data["token"] = token;
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
    if (errorMessage != 0) {
      *errorMessage = json.error().errorMessage();
    }
    return QByteArray();
  }

  if (errorMessage != 0) {
    errorMessage->clear();
  }
  return payload;
}

int DiscordJsonParser::valueToInt(const QVariant &value, int fallback) {
  bool ok = false;
  int result = value.toInt(&ok);
  return ok ? result : fallback;
}

bool DiscordJsonParser::hasJsonToken(const QByteArray &bytes,
                                     const char *compactToken,
                                     const char *spacedToken) {
  return bytes.indexOf(compactToken) >= 0 || bytes.indexOf(spacedToken) >= 0;
}

bool DiscordJsonParser::isLargeReadyPayload(const QByteArray &bytes) {
  return bytes.size() > 262144 &&
         hasJsonToken(bytes, "\"op\":0", "\"op\": 0") &&
         hasJsonToken(bytes, "\"t\":\"READY\"", "\"t\": \"READY\"");
}

QString DiscordJsonParser::extractStringField(const QByteArray &bytes,
                                              const char *fieldName) {
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
