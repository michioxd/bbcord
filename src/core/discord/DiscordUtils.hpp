#ifndef DiscordUtils_HPP_
#define DiscordUtils_HPP_

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace DiscordUtils {

QByteArray desktopUserAgent();
QByteArray desktopUserAgentHeader();

QString firstLetter(const QString &text);
QString firstTwoWordLetters(const QString &text);
QString cleanSnowflake(const QVariant &value);
void appendUniqueGuildId(QStringList *guildIds, const QString &guildId);
void appendGuildIdsFromUserSettingsProto(QStringList *orderedGuildIds,
                                         const QString &base64Proto);
QVariantList guildFoldersFromUserSettingsProto(const QString &base64Proto);
bool positionShouldMoveBefore(const QVariantMap &left,
                              const QVariantMap &right);
bool dmShouldMoveBefore(const QVariantMap &left, const QVariantMap &right);
void stableSortItems(QVariantList *items,
                     bool (*shouldMoveBefore)(const QVariantMap &left,
                                              const QVariantMap &right));

} // namespace DiscordUtils

#endif /* DiscordUtils_HPP_ */
