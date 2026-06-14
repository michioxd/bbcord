#ifndef JsonParser_HPP_
#define JsonParser_HPP_

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class DiscordJsonParser {
public:
  struct GatewayPayload {
    GatewayPayload() : op(-1), sequence(-1), valid(false) {}

    int op;
    int sequence;
    QString eventName;
    QVariantMap data;
    QString errorMessage;
    bool valid;
  };

  static GatewayPayload parseGatewayPayload(const QByteArray &bytes);
  static QVariantMap parseObject(const QByteArray &bytes,
                                 QString *errorMessage = 0);
  static QVariantList parseArray(const QByteArray &bytes,
                                 QString *errorMessage = 0);
  static QByteArray buildIdentifyPayload(const QString &token,
                                         QString *errorMessage = 0);
  static QByteArray buildGuildSubscribePayload(const QString &guildId,
                                               const QString &channelId,
                                               int rangeStart, int rangeEnd,
                                               QString *errorMessage = 0);
  static QByteArray buildGuildMembersRequestPayload(const QString &guildId,
                                                    const QString &query,
                                                    int limit,
                                                    const QString &nonce,
                                                    QString *errorMessage = 0);
  static int valueToInt(const QVariant &value, int fallback);
  static bool hasJsonToken(const QByteArray &bytes, const char *compactToken,
                           const char *spacedToken);
  static bool isLargeReadyPayload(const QByteArray &bytes);
  static QString extractStringField(const QByteArray &bytes,
                                    const char *fieldName);
  static bool extractBoolField(const QByteArray &bytes, const char *fieldName,
                               bool fallback = false);
  static QByteArray extractObjectField(const QByteArray &bytes,
                                       const char *fieldName);
  static QVariantList extractArrayField(const QByteArray &bytes,
                                        const char *fieldName);
};

#endif /* JsonParser_HPP_ */
