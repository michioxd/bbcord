#include "SortUtils.hpp"

#include "../discord/DiscordUtils.hpp"
#include "../models/Models.hpp"

#include <QDebug>
#include <QMap>

namespace {
const int kGuildStageVoiceChannelType = 13;

bool isVoiceLikeChannel(const QVariantMap &item) {
  int channelType = item.value("channelType").toInt();
  return channelType == DiscordChannel::GuildVoice ||
         channelType == kGuildStageVoiceChannelType;
}

bool channelShouldMoveBefore(const QVariantMap &left,
                             const QVariantMap &right) {
  bool leftVoiceLike = isVoiceLikeChannel(left);
  bool rightVoiceLike = isVoiceLikeChannel(right);
  if (leftVoiceLike != rightVoiceLike) {
    return !leftVoiceLike;
  }

  return DiscordUtils::positionShouldMoveBefore(left, right);
}
} // namespace

SortUtils::SortUtils(QObject *parent) : QObject(parent) {}

SortUtils::~SortUtils() {}

bool SortUtils::applyGuildOrder(QVariantList &guilds,
                                const QStringList &orderedGuildIds) {
  if (orderedGuildIds.isEmpty() || guilds.isEmpty()) {
    return false;
  }

  QVariantList ordered;
  QStringList usedIds;
  for (int i = 0; i < orderedGuildIds.size(); ++i) {
    QString guildId = orderedGuildIds.at(i).trimmed();
    if (guildId.isEmpty() || usedIds.contains(guildId)) {
      continue;
    }

    for (int j = 0; j < guilds.size(); ++j) {
      QVariantMap guild = guilds.at(j).toMap();
      if (guild.value("id").toString() == guildId) {
        ordered.append(guild);
        usedIds.append(guildId);
        break;
      }
    }
  }

  if (ordered.isEmpty()) {
    return false;
  }

  for (int i = 0; i < guilds.size(); ++i) {
    QVariantMap guild = guilds.at(i).toMap();
    if (!usedIds.contains(guild.value("id").toString())) {
      ordered.append(guild);
    }
  }

  guilds = ordered;
  return true;
}

bool SortUtils::applyGuildOrderFromGatewayPayload(QVariantList &guilds,
                                                  QStringList &orderedGuildIds,
                                                  const QVariantMap &payload) {
  QStringList newOrderedGuildIds;

  DiscordUtils::appendGuildIdsFromUserSettingsProto(
      &newOrderedGuildIds, payload.value("user_settings_proto").toString());

  QVariantMap settings = payload.value("settings").toMap();
  DiscordUtils::appendGuildIdsFromUserSettingsProto(
      &newOrderedGuildIds, settings.value("proto").toString());

  QVariantList folders = payload.value("guild_folders").toList();
  if (folders.isEmpty()) {
    QVariantMap userSettings = payload.value("user_settings").toMap();
    folders = userSettings.value("guild_folders").toList();
  }

  for (int i = 0; i < folders.size(); ++i) {
    QVariantMap folder = folders.at(i).toMap();
    QVariantList guildIds = folder.value("guild_ids").toList();
    for (int j = 0; j < guildIds.size(); ++j) {
      DiscordUtils::appendUniqueGuildId(&newOrderedGuildIds,
                                        guildIds.at(j).toString());
    }
  }

  if (newOrderedGuildIds.isEmpty()) {
    return false;
  }

  orderedGuildIds = newOrderedGuildIds;
  qDebug() << "[discord-client] applied guild order from gateway"
           << orderedGuildIds.size();

  return applyGuildOrder(guilds, orderedGuildIds);
}

QVariantList
SortUtils::sortedAccessibleGuildChannels(const QVariantList &channels) const {
  QVariantList categories;
  QVariantList rootChannels;
  QMap<QString, QVariantList> channelsByCategory;
  QStringList categoryIds;

  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = channels.at(i).toMap();
    if (item.contains("accessible") && !item.value("accessible").toBool()) {
      continue;
    }
    if (item.value("type").toString() == "category") {
      categories.append(item);
      categoryIds.append(item.value("id").toString());
    }
  }

  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap item = channels.at(i).toMap();
    if (item.contains("accessible") && !item.value("accessible").toBool()) {
      continue;
    }
    if (item.value("type").toString() == "category") {
      continue;
    }

    QString parentId = item.value("parentId").toString();
    if (!parentId.isEmpty() && categoryIds.contains(parentId)) {
      QVariantList categoryChannels = channelsByCategory.value(parentId);
      categoryChannels.append(item);
      channelsByCategory.insert(parentId, categoryChannels);
    } else {
      rootChannels.append(item);
    }
  }

  DiscordUtils::stableSortItems(&categories,
                                DiscordUtils::positionShouldMoveBefore);
  DiscordUtils::stableSortItems(&rootChannels, channelShouldMoveBefore);

  QVariantList result;
  for (int i = 0; i < rootChannels.size(); ++i) {
    result.append(rootChannels.at(i));
  }

  for (int i = 0; i < categories.size(); ++i) {
    QVariantMap item = categories.at(i).toMap();
    result.append(item);

    QVariantList categoryChannels =
        channelsByCategory.value(item.value("id").toString());
    DiscordUtils::stableSortItems(&categoryChannels, channelShouldMoveBefore);
    for (int j = 0; j < categoryChannels.size(); ++j) {
      result.append(categoryChannels.at(j));
    }
  }

  return result;
}
