#include "ChannelMemberList.hpp"

#include "../core/AppStore.hpp"
#include "../core/Client.hpp"

#include <QCryptographicHash>
#include <QDebug>
#include <QMap>
#include <QPair>
#include <QTimer>
#include <QVariant>
#include <QtAlgorithms>

namespace {
const int kMemberPageSize = 100;
const quint64 kPermissionViewChannel = 0x00000400ULL;
const quint64 kPermissionAdministrator = 0x00000008ULL;

QString stringValue(const QVariantMap &map, const QString &key) {
  return map.value(key).toString().trimmed();
}

quint64 permissionValue(const QVariant &value) {
  bool ok = false;
  quint64 result = value.toString().toULongLong(&ok);
  if (ok) {
    return result;
  }
  return static_cast<quint64>(value.toULongLong(&ok));
}

bool rolePositionGreater(const QPair<int, QString> &left,
                         const QPair<int, QString> &right) {
  if (left.first == right.first) {
    return left.second < right.second;
  }
  return left.first > right.first;
}

typedef QPair<int, QString> RolePosition;

bool memberNameLess(const QVariant &left, const QVariant &right) {
  return left.toMap().value("name").toString().toLower() <
         right.toMap().value("name").toString().toLower();
}

QString overwriteId(const QVariantMap &overwrite) {
  QString id = overwrite.value("id").toString().trimmed();
  if (!id.isEmpty()) {
    return id;
  }
  return overwrite.value("role_id").toString().trimmed();
}
} // namespace

ChannelMemberListController::ChannelMemberListController(DiscordClient *client,
                                                         AppStore *store,
                                                         QObject *parent)
    : QObject(parent), m_client(client), m_store(store),
      m_memberDataModel(new bb::cascades::ArrayDataModel(this)),
      m_expectedMemberCount(-1), m_loadedCount(0), m_nextRangeStart(0),
      m_lastRequestedStart(0), m_lastRequestedEnd(-1), m_loading(false),
      m_hasMore(true), m_usingChannelMemberList(false) {
  if (m_client) {
    connect(m_client, SIGNAL(gatewayDispatchReceived(QString, QVariantMap)),
            this, SLOT(onGatewayDispatch(QString, QVariantMap)));
    connect(m_client, SIGNAL(gatewayConnectionClosed()), this,
            SLOT(onGatewayClosed()));
  }
  if (m_store) {
    connect(m_store, SIGNAL(chatAvatarChanged(QString, QString)), this,
            SLOT(onChatAvatarChanged(QString, QString)));
  }
}

QString ChannelMemberListController::currentChannelId() const {
  return m_currentChannelId;
}

QString ChannelMemberListController::currentGuildId() const {
  return m_currentGuildId;
}

QString ChannelMemberListController::currentChannelName() const {
  return m_currentChannelName;
}

bool ChannelMemberListController::loading() const { return m_loading; }

bb::cascades::DataModel *ChannelMemberListController::memberDataModel() const {
  return m_memberDataModel;
}

void ChannelMemberListController::openChannel(const QString &channelId,
                                              const QString &guildId,
                                              const QString &channelName) {
  QString safeChannelId = channelId.trimmed();
  QString safeGuildId = guildId.trimmed();
  if (safeChannelId.isEmpty() || safeGuildId.isEmpty()) {
    qDebug() << "[member-list] openChannel ignored; missing ids"
             << "channel" << safeChannelId << "guild" << safeGuildId;
    return;
  }

  qDebug() << "[member-list] openChannel"
           << "channel" << safeChannelId << "guild" << safeGuildId << "name"
           << channelName;

  bool changed = m_currentChannelId != safeChannelId ||
                 m_currentGuildId != safeGuildId ||
                 m_currentChannelName != channelName;
  m_currentChannelId = safeChannelId;
  m_currentGuildId = safeGuildId;
  m_currentChannelName = channelName;
  applyGuildMetadata(m_guildMetadataById.value(safeGuildId));
  m_membersByUserId.clear();
  m_errorText.clear();
  m_modelIndexByUserId.clear();
  m_requestedAvatarUserIds.clear();
  m_channelMemberUserIds.clear();
  m_expectedMemberCount = -1;
  m_loadedCount = 0;
  m_nextRangeStart = 0;
  m_lastRequestedStart = 0;
  m_lastRequestedEnd = -1;
  m_hasMore = true;
  m_usingChannelMemberList = false;
  rebuildModel();
  if (m_lastListUpdateByChannelId.contains(safeChannelId)) {
    qDebug() << "[member-list] applying cached member list update"
             << "guild" << safeGuildId << "channel" << safeChannelId;
    onGatewayDispatch("GUILD_MEMBER_LIST_UPDATE",
                      m_lastListUpdateByChannelId.value(safeChannelId));
    if (m_loadedCount > 0) {
      m_nextRangeStart = m_loadedCount;
    }
  }
  if (changed) {
    emit currentChannelChanged();
  }
  if (m_loadedCount == 0) {
    loadMore();
  }
}

void ChannelMemberListController::closeChannel() {
  m_currentChannelId.clear();
  m_currentGuildId.clear();
  m_currentChannelName.clear();
  m_membersByUserId.clear();
  m_errorText.clear();
  m_modelIndexByUserId.clear();
  m_requestedAvatarUserIds.clear();
  m_channelMemberUserIds.clear();
  m_expectedMemberCount = -1;
  m_loadedCount = 0;
  m_nextRangeStart = 0;
  m_lastRequestedStart = 0;
  m_lastRequestedEnd = -1;
  m_hasMore = true;
  m_usingChannelMemberList = false;
  setLoading(false);
  rebuildModel();
  emit currentChannelChanged();
}

void ChannelMemberListController::loadMore() {
  if (m_loading || !m_hasMore || m_currentChannelId.isEmpty() ||
      m_currentGuildId.isEmpty()) {
    qDebug() << "[member-list] loadMore ignored"
             << "loading" << m_loading << "hasMore" << m_hasMore << "channel"
             << m_currentChannelId << "guild" << m_currentGuildId;
    return;
  }

  int start = m_nextRangeStart;
  requestRange(start, start + kMemberPageSize - 1);
}

void ChannelMemberListController::loadMemberAvatar(const QString &userId,
                                                   const QString &avatarHash) {
  QString safeUserId = userId.trimmed();
  QString safeAvatarHash = avatarHash.trimmed();
  if (safeUserId.isEmpty()) {
    return;
  }

  QString source =
      m_client ? m_client->avatarSourceForUser(safeUserId) : QString();
  if (!source.isEmpty()) {
    m_avatarSourcesByUserId.insert(safeUserId, source);
    if (m_membersByUserId.contains(safeUserId)) {
      QVariantMap member = m_membersByUserId.value(safeUserId);
      member["avatar"] = source;
      m_membersByUserId.insert(safeUserId, member);
      updateMemberRow(safeUserId);
    }
    return;
  }
  if (m_client && !safeAvatarHash.isEmpty() &&
      !m_requestedAvatarUserIds.contains(safeUserId)) {
    m_requestedAvatarUserIds.insert(safeUserId);
    m_client->loadUserAvatar(safeUserId, safeAvatarHash);
  }
}

void ChannelMemberListController::onGatewayDispatch(
    const QString &eventName, const QVariantMap &payload) {
  if (eventName == "PRESENCE_UPDATE") {
    QString userId = payload.value("user").toMap().value("id").toString();
    if (!userId.isEmpty()) {
      QVariantMap presence = payload;
      QString status = presence.value("status").toString().trimmed();
      presence["status"] = status.isEmpty() ? QString("offline") : status;
      m_presencesByUserId.insert(userId, presence);
    }
    if (m_membersByUserId.contains(userId)) {
      QVariantMap member = m_membersByUserId.value(userId);
      QString status = presenceForUser(userId).value("status").toString();
      member["status"] = displayStatus(userId);
      member["statusColor"] = statusColor(status);
      m_membersByUserId.insert(userId, member);
      updateMemberRow(userId);
    }
    return;
  }

  if (eventName == "RATE_LIMITED") {
    QVariantMap meta = payload.value("meta").toMap();
    int opcode = payload.value("opcode").toInt();
    QString guildId = meta.value("guild_id").toString();
    if (opcode == 8 && guildId == m_currentGuildId && m_loading) {
      qDebug() << "[member-list] member request rate limited"
               << "guild" << guildId << "retry_after"
               << payload.value("retry_after").toDouble();
      m_errorText = tr("Member list is rate limited. Try again later.");
      m_hasMore = false;
      setLoading(false);
      rebuildModel();
    }
    return;
  }

  if (eventName == "GUILD_CREATE") {
    cacheGuildMetadata(payload);
    if (payload.value("id").toString() == m_currentGuildId) {
      applyGuildMetadata(payload);
      if (m_memberDataModel->size() == 0 || !m_membersByUserId.isEmpty()) {
        rebuildModel();
      }
    }
    return;
  }

  if (eventName == "GUILD_MEMBERS_CHUNK" &&
      payload.value("guild_id").toString() == m_currentGuildId) {
    QVariantList members = payload.value("members").toList();
    QVariantList presences = payload.value("presences").toList();
    qDebug() << "[member-list] GUILD_MEMBERS_CHUNK received"
             << "guild" << m_currentGuildId << "members" << members.size()
             << "presences" << presences.size();

    bool wasLoading = m_loading;
    m_errorText.clear();
    for (int i = 0; i < presences.size(); ++i) {
      QVariantMap presence = presences.at(i).toMap();
      QString userId = presence.value("user").toMap().value("id").toString();
      QString status = presence.value("status").toString().trimmed();
      if (!userId.isEmpty()) {
        presence["status"] = status.isEmpty() ? QString("offline") : status;
        m_presencesByUserId.insert(userId, presence);
      }
    }
    for (int i = 0; i < members.size(); ++i) {
      QVariantMap memberItem = memberItemFromPayload(members.at(i).toMap());
      QString userId = memberItem.value("userId").toString();
      if (!userId.isEmpty()) {
        m_membersByUserId.insert(userId, memberItem);
      }
    }

    if (wasLoading && members.size() < kMemberPageSize) {
      m_hasMore = false;
    }
    if (!m_usingChannelMemberList) {
      m_hasMore = false;
    }
    setLoading(false);
    if (!m_usingChannelMemberList) {
      rebuildModel();
    }
    return;
  }

  if (eventName == "CHANNEL_MEMBER_COUNT_UPDATE" &&
      payload.value("guild_id").toString() == m_currentGuildId &&
      payload.value("channel_id").toString() == m_currentChannelId) {
    qDebug() << "[member-list] CHANNEL_MEMBER_COUNT_UPDATE received"
             << "guild" << m_currentGuildId << "channel" << m_currentChannelId
             << "members" << payload.value("member_count").toInt()
             << "presences" << payload.value("presence_count").toInt();
    int memberCount = payload.value("member_count").toInt();
    if (memberCount > 0) {
      m_expectedMemberCount = memberCount;
    }
    return;
  }

  if (eventName != "GUILD_MEMBER_LIST_UPDATE") {
    return;
  }

  QString updateGuildId = payload.value("guild_id").toString();
  QString updateChannelId = payload.value("channel_id").toString();
  if (!updateChannelId.isEmpty()) {
    m_lastListUpdateByChannelId.insert(updateChannelId, payload);
  }

  if (updateGuildId != m_currentGuildId ||
      (!updateChannelId.isEmpty() && updateChannelId != m_currentChannelId)) {
    return;
  }

  qDebug() << "[member-list] GUILD_MEMBER_LIST_UPDATE received"
           << "guild" << m_currentGuildId << "ops"
           << payload.value("ops").toList().size();

  bool wasLoading = m_loading;
  bool changed = false;
  int receivedMembers = 0;
  m_errorText.clear();
  QVariantList ops = payload.value("ops").toList();
  for (int i = 0; i < ops.size(); ++i) {
    QVariantMap op = ops.at(i).toMap();
    QVariantList items = op.value("items").toList();
    for (int j = 0; j < items.size(); ++j) {
      QVariantMap item = items.at(j).toMap();
      QVariantMap member = item.value("member").toMap();
      if (member.isEmpty()) {
        continue;
      }
      receivedMembers++;
      QVariantMap memberItem = memberItemFromPayload(member);
      QString userId = memberItem.value("userId").toString();
      if (!userId.isEmpty()) {
        if (!m_channelMemberUserIds.contains(userId) ||
            !m_membersByUserId.contains(userId)) {
          changed = true;
        }
        m_channelMemberUserIds.insert(userId);
        m_membersByUserId.insert(userId, memberItem);
      }
    }
  }

  if (receivedMembers > 0) {
    m_usingChannelMemberList = true;
    m_loadedCount = m_channelMemberUserIds.size();
  }

  if (wasLoading && receivedMembers < kMemberPageSize) {
    m_hasMore = false;
  }
  if (m_expectedMemberCount > 0 &&
      m_channelMemberUserIds.size() >= m_expectedMemberCount) {
    m_hasMore = false;
  }
  setLoading(false);
  if (changed || wasLoading) {
    rebuildModel();
  }
}

void ChannelMemberListController::onGatewayClosed() {
  if (m_currentChannelId.isEmpty() || !m_loading) {
    return;
  }

  m_errorText = tr("Unable to load members. Gateway disconnected.");
  m_hasMore = false;
  setLoading(false);
  rebuildModel();
}

void ChannelMemberListController::onLoadTimeout() {
  if (!m_loading || m_currentChannelId.isEmpty()) {
    return;
  }

  qDebug() << "[member-list] load timeout"
           << "guild" << m_currentGuildId << "channel" << m_currentChannelId
           << "range" << m_lastRequestedStart << m_lastRequestedEnd;
  m_errorText = tr("Unable to load members. No gateway response.");
  m_hasMore = false;
  setLoading(false);
  rebuildModel();
}

void ChannelMemberListController::onChatAvatarChanged(
    const QString &userId, const QString &avatarSource) {
  if (!m_membersByUserId.contains(userId)) {
    return;
  }

  m_requestedAvatarUserIds.remove(userId);
  m_avatarSourcesByUserId.insert(userId, avatarSource);
  QVariantMap member = m_membersByUserId.value(userId);
  member["avatar"] = avatarSource;
  m_membersByUserId.insert(userId, member);
  updateMemberRow(userId);
}

QVariantMap ChannelMemberListController::memberItemFromPayload(
    const QVariantMap &member) const {
  QVariantMap user = member.value("user").toMap();
  QString userId = stringValue(user, "id");
  QString avatarHash = stringValue(user, "avatar");
  QString name = displayName(member, user);
  QVariantMap item;
  item["type"] = "member";
  item["userId"] = userId;
  item["avatarHash"] = avatarHash;
  item["name"] = name;
  item["initials"] = initialsForName(name);
  item["avatarColor"] = avatarColor(userId);
  item["avatar"] = m_avatarSourcesByUserId.value(userId);
  if (item.value("avatar").toString().isEmpty() && m_client) {
    QString source = m_client->avatarSourceForUser(userId);
    if (!source.isEmpty()) {
      item["avatar"] = source;
    }
  }
  item["nameColor"] = "#F2F3F5";
  item["roleIds"] = member.value("roles").toList();
  item["topRoleId"] = topRoleIdForMember(item);
  item["status"] = displayStatus(userId);
  item["statusColor"] =
      statusColor(presenceForUser(userId).value("status").toString());
  return item;
}

QVariantMap ChannelMemberListController::roleItem(const QString &name,
                                                  int count) const {
  QVariantMap item;
  item["type"] = "role";
  item["name"] = name;
  item["count"] = count;
  return item;
}

void ChannelMemberListController::cacheGuildMetadata(const QVariantMap &guild) {
  QString guildId = guild.value("id").toString().trimmed();
  if (guildId.isEmpty()) {
    return;
  }
  m_guildMetadataById.insert(guildId, guild);
}

void ChannelMemberListController::applyGuildMetadata(const QVariantMap &guild) {
  m_rolesById.clear();
  m_roleIdsByOrder.clear();
  m_currentChannelOverwrites.clear();
  m_guildOwnerId.clear();

  if (guild.isEmpty()) {
    return;
  }

  m_guildOwnerId = guild.value("owner_id").toString().trimmed();

  QList<RolePosition> roleOrder;
  QVariantList roles = guild.value("roles").toList();
  for (int i = 0; i < roles.size(); ++i) {
    QVariantMap role = roles.at(i).toMap();
    QString roleId = role.value("id").toString().trimmed();
    if (roleId.isEmpty()) {
      continue;
    }
    m_rolesById.insert(roleId, role);
    roleOrder.append(qMakePair(role.value("position").toInt(), roleId));
  }
  qSort(roleOrder.begin(), roleOrder.end(), rolePositionGreater);
  for (int i = 0; i < roleOrder.size(); ++i) {
    m_roleIdsByOrder.append(roleOrder.at(i).second);
  }

  QVariantList channels = guild.value("channels").toList();
  for (int i = 0; i < channels.size(); ++i) {
    QVariantMap channel = channels.at(i).toMap();
    if (channel.value("id").toString() == m_currentChannelId) {
      m_currentChannelOverwrites =
          channel.value("permission_overwrites").toList();
      break;
    }
  }
}

bool ChannelMemberListController::memberCanAccessCurrentChannel(
    const QVariantMap &member) const {
  QString userId = member.value("userId").toString();
  if (!m_guildOwnerId.isEmpty() && userId == m_guildOwnerId) {
    return true;
  }

  if (m_rolesById.isEmpty() || m_currentGuildId.isEmpty()) {
    return true;
  }

  quint64 permissions =
      permissionValue(m_rolesById.value(m_currentGuildId).value("permissions"));
  QVariantList memberRoles = member.value("roleIds").toList();
  QSet<QString> memberRoleIds;
  for (int i = 0; i < memberRoles.size(); ++i) {
    QString roleId = memberRoles.at(i).toString().trimmed();
    if (roleId.isEmpty()) {
      continue;
    }
    memberRoleIds.insert(roleId);
    permissions |=
        permissionValue(m_rolesById.value(roleId).value("permissions"));
  }

  if ((permissions & kPermissionAdministrator) == kPermissionAdministrator) {
    return true;
  }

  for (int i = 0; i < m_currentChannelOverwrites.size(); ++i) {
    QVariantMap overwrite = m_currentChannelOverwrites.at(i).toMap();
    if (overwriteId(overwrite) != m_currentGuildId) {
      continue;
    }
    permissions &= ~permissionValue(overwrite.value("deny"));
    permissions |= permissionValue(overwrite.value("allow"));
    break;
  }

  quint64 roleDeny = 0;
  quint64 roleAllow = 0;
  for (int i = 0; i < m_currentChannelOverwrites.size(); ++i) {
    QVariantMap overwrite = m_currentChannelOverwrites.at(i).toMap();
    QString type = overwrite.value("type").toString();
    QString id = overwriteId(overwrite);
    bool roleOverwrite = type == "0" || type.toLower() == "role" ||
                         (type.isEmpty() && m_rolesById.contains(id));
    if (!roleOverwrite || !memberRoleIds.contains(id)) {
      continue;
    }
    roleDeny |= permissionValue(overwrite.value("deny"));
    roleAllow |= permissionValue(overwrite.value("allow"));
  }
  permissions &= ~roleDeny;
  permissions |= roleAllow;

  for (int i = 0; i < m_currentChannelOverwrites.size(); ++i) {
    QVariantMap overwrite = m_currentChannelOverwrites.at(i).toMap();
    QString type = overwrite.value("type").toString();
    QString id = overwriteId(overwrite);
    bool memberOverwrite =
        type == "1" || type.toLower() == "member" ||
        (type.isEmpty() && id == userId && !m_rolesById.contains(id));
    if (!memberOverwrite || id != userId) {
      continue;
    }
    permissions &= ~permissionValue(overwrite.value("deny"));
    permissions |= permissionValue(overwrite.value("allow"));
    break;
  }

  return (permissions & kPermissionViewChannel) == kPermissionViewChannel;
}

QString ChannelMemberListController::topRoleIdForMember(
    const QVariantMap &member) const {
  QVariantList memberRoles = member.value("roleIds").toList();
  QSet<QString> memberRoleIds;
  for (int i = 0; i < memberRoles.size(); ++i) {
    memberRoleIds.insert(memberRoles.at(i).toString());
  }

  for (int i = 0; i < m_roleIdsByOrder.size(); ++i) {
    QString roleId = m_roleIdsByOrder.at(i);
    if (roleId == m_currentGuildId) {
      continue;
    }
    if (memberRoleIds.contains(roleId)) {
      return roleId;
    }
  }
  return QString();
}

QVariantMap
ChannelMemberListController::presenceForUser(const QString &userId) const {
  QString safeUserId = userId.trimmed();
  if (safeUserId.isEmpty()) {
    return QVariantMap();
  }
  if (m_presencesByUserId.contains(safeUserId)) {
    return m_presencesByUserId.value(safeUserId);
  }
  return m_client ? m_client->guildPresenceForUser(safeUserId) : QVariantMap();
}

bool ChannelMemberListController::isMemberOnline(const QString &userId) const {
  QString status = presenceForUser(userId).value("status").toString();
  return !status.isEmpty() && status != "offline";
}

QString
ChannelMemberListController::displayStatus(const QString &userId) const {
  QVariantMap presence = presenceForUser(userId);
  QString activity = activityText(presence);
  if (!activity.isEmpty()) {
    return activity;
  }

  QString status = presence.value("status").toString();
  if (status == "online")
    return "Online";
  if (status == "idle")
    return "Idle";
  if (status == "dnd")
    return "Do Not Disturb";
  return "Offline";
}

QString ChannelMemberListController::statusColor(const QString &status) const {
  if (status == "online")
    return "#23A55A";
  if (status == "idle")
    return "#F0B232";
  if (status == "dnd")
    return "#F23F43";
  return "#80848E";
}

QString
ChannelMemberListController::displayName(const QVariantMap &member,
                                         const QVariantMap &user) const {
  QString nick = stringValue(member, "nick");
  if (!nick.isEmpty()) {
    return nick;
  }
  QString globalName = stringValue(user, "global_name");
  if (!globalName.isEmpty()) {
    return globalName;
  }
  return stringValue(user, "username");
}

QString
ChannelMemberListController::initialsForName(const QString &name) const {
  QString trimmed = name.trimmed();
  if (trimmed.isEmpty()) {
    return "?";
  }
  return trimmed.left(1).toUpper();
}

QString ChannelMemberListController::avatarColor(const QString &userId) const {
  QByteArray hash =
      QCryptographicHash::hash(userId.toUtf8(), QCryptographicHash::Md5)
          .toHex();
  if (hash.size() < 6) {
    return "#5865F2";
  }
  return QString("#") + QString::fromLatin1(hash.left(6));
}

QString
ChannelMemberListController::activityText(const QVariantMap &presence) const {
  QVariantList activities = presence.value("activities").toList();
  if (activities.isEmpty()) {
    return QString();
  }

  QVariantMap activity = activities.first().toMap();
  QString state = stringValue(activity, "state");
  if (!state.isEmpty()) {
    return state;
  }
  QString name = stringValue(activity, "name");
  if (!name.isEmpty()) {
    return name;
  }
  return QString();
}

void ChannelMemberListController::requestRange(int start, int end) {
  m_lastRequestedStart = start;
  m_lastRequestedEnd = end;
  m_nextRangeStart = end + 1;
  setLoading(true);
  if (m_memberDataModel->size() == 0) {
    rebuildModel();
  } else {
    QVariantMap lastItem =
        m_memberDataModel->value(m_memberDataModel->size() - 1).toMap();
    if (lastItem.value("type").toString() != "loading") {
      QVariantMap loading;
      loading["type"] = "loading";
      m_memberDataModel->append(loading);
    }
  }
  qDebug() << "[member-list] request range"
           << "guild" << m_currentGuildId << "channel" << m_currentChannelId
           << "start" << start << "end" << end << "hasClient"
           << (m_client != 0);
  QTimer::singleShot(12000, this, SLOT(onLoadTimeout()));
  if (!m_client) {
    m_errorText = tr("Unable to load members. Client is not ready.");
    m_hasMore = false;
    setLoading(false);
    rebuildModel();
    return;
  }
  m_client->subscribeChannelMembers(m_currentChannelId, m_currentGuildId, start,
                                    end);
}

void ChannelMemberListController::rebuildModel() {
  m_memberDataModel->clear();
  m_modelIndexByUserId.clear();

  QHash<QString, QVariantList> onlineByRoleId;
  QHash<QString, QVariantList> offlineByRoleId;
  QVariantList onlineWithoutRole;
  QVariantList offlineWithoutRole;
  QList<QString> userIds = m_membersByUserId.keys();
  for (int i = 0; i < userIds.size(); ++i) {
    if (m_usingChannelMemberList &&
        !m_channelMemberUserIds.contains(userIds.at(i))) {
      continue;
    }

    QVariantMap member = m_membersByUserId.value(userIds.at(i));
    member["status"] = displayStatus(userIds.at(i));
    member["statusColor"] =
        statusColor(presenceForUser(userIds.at(i)).value("status").toString());
    member["topRoleId"] = topRoleIdForMember(member);
    m_membersByUserId.insert(userIds.at(i), member);

    if (!memberCanAccessCurrentChannel(member)) {
      continue;
    }

    QString topRoleId = member.value("topRoleId").toString();
    bool online = isMemberOnline(userIds.at(i));
    if (topRoleId.isEmpty()) {
      if (online) {
        onlineWithoutRole.append(member);
      } else {
        offlineWithoutRole.append(member);
      }
    } else {
      if (online) {
        onlineByRoleId[topRoleId].append(member);
      } else {
        offlineByRoleId[topRoleId].append(member);
      }
    }
  }

  appendMemberGroups(tr("Online"), onlineByRoleId, onlineWithoutRole);
  appendMemberGroups(tr("Offline"), offlineByRoleId, offlineWithoutRole);

  bool hasVisibleMembers = !m_modelIndexByUserId.isEmpty();
  if (m_loading) {
    QVariantMap loading;
    loading["type"] = "loading";
    m_memberDataModel->append(loading);
  } else if (!hasVisibleMembers && !m_errorText.isEmpty()) {
    QVariantMap empty;
    empty["type"] = "empty";
    empty["text"] = m_errorText;
    m_memberDataModel->append(empty);
  }
  emit memberDataModelChanged();
}

void ChannelMemberListController::appendMemberGroups(
    const QString &statusName,
    const QHash<QString, QVariantList> &membersByRoleId,
    const QVariantList &membersWithoutRole) {
  for (int i = 0; i < m_roleIdsByOrder.size(); ++i) {
    QString roleId = m_roleIdsByOrder.at(i);
    if (roleId == m_currentGuildId || !membersByRoleId.contains(roleId)) {
      continue;
    }
    QVariantList roleMembers = membersByRoleId.value(roleId);
    qSort(roleMembers.begin(), roleMembers.end(), memberNameLess);
    QString roleName = m_rolesById.value(roleId).value("name").toString();
    if (roleName.isEmpty()) {
      roleName = tr("Members");
    }
    m_memberDataModel->append(roleItem(
        QString("%1 — %2").arg(statusName).arg(roleName), roleMembers.size()));
    for (int j = 0; j < roleMembers.size(); ++j) {
      QVariantMap member = roleMembers.at(j).toMap();
      m_modelIndexByUserId.insert(member.value("userId").toString(),
                                  m_memberDataModel->size());
      m_memberDataModel->append(member);
    }
  }

  if (membersWithoutRole.isEmpty()) {
    return;
  }

  QVariantList sortedMembers = membersWithoutRole;
  qSort(sortedMembers.begin(), sortedMembers.end(), memberNameLess);
  m_memberDataModel->append(
      roleItem(QString("%1 — %2").arg(statusName).arg(tr("Members")),
               sortedMembers.size()));
  for (int i = 0; i < sortedMembers.size(); ++i) {
    QVariantMap member = sortedMembers.at(i).toMap();
    m_modelIndexByUserId.insert(member.value("userId").toString(),
                                m_memberDataModel->size());
    m_memberDataModel->append(member);
  }
}

void ChannelMemberListController::updateMemberRow(const QString &userId) {
  if (!m_membersByUserId.contains(userId)) {
    return;
  }

  QVariantMap member = m_membersByUserId.value(userId);
  member["status"] = displayStatus(userId);
  member["statusColor"] = statusColor(
      m_client
          ? m_client->guildPresenceForUser(userId).value("status").toString()
          : QString());
  member["topRoleId"] = topRoleIdForMember(member);
  if (member.value("avatar").toString().isEmpty()) {
    member["avatar"] = m_avatarSourcesByUserId.value(userId);
  }
  m_membersByUserId.insert(userId, member);

  if (!memberCanAccessCurrentChannel(member)) {
    if (m_modelIndexByUserId.contains(userId)) {
      rebuildModel();
    }
    return;
  }

  if (!m_modelIndexByUserId.contains(userId)) {
    return;
  }

  int index = m_modelIndexByUserId.value(userId);
  if (index < 0 || index >= m_memberDataModel->size()) {
    rebuildModel();
    return;
  }

  QVariantMap current = m_memberDataModel->value(index).toMap();
  if (current.value("type").toString() != "member" ||
      current.value("userId").toString() != userId) {
    rebuildModel();
    return;
  }

  m_memberDataModel->replace(index, member);
}

void ChannelMemberListController::setLoading(bool loading) {
  if (m_loading == loading) {
    return;
  }
  m_loading = loading;
  emit loadingChanged();
}
