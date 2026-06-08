#include "Models.hpp"

#include <QDateTime>

namespace {
QDateTime parseDiscordTimestamp(const QString &timestamp) {
  QString value = timestamp.trimmed();
  if (value.isEmpty()) {
    return QDateTime();
  }

  bool utc = false;
  if (value.endsWith("Z")) {
    utc = true;
    value.chop(1);
  }

  int plusIndex = value.indexOf('+', 10);
  int minusIndex = value.indexOf('-', 10);
  int zoneIndex = plusIndex >= 0 ? plusIndex : minusIndex;
  QString zone;
  if (zoneIndex >= 0) {
    zone = value.mid(zoneIndex);
    value = value.left(zoneIndex);
  }

  int dotIndex = value.indexOf('.');
  QString base = dotIndex >= 0 ? value.left(dotIndex) : value;
  QString msecs;
  if (dotIndex >= 0) {
    msecs = value.mid(dotIndex + 1);
    while (msecs.length() > 3) {
      msecs.chop(1);
    }
    while (msecs.length() < 3) {
      msecs.append('0');
    }
  }

  QDateTime parsed = QDateTime::fromString(base, "yyyy-MM-dd'T'HH:mm:ss");
  if (!parsed.isValid()) {
    parsed = QDateTime::fromString(base, "yyyy-MM-dd'T'HH:mm");
  }
  if (!parsed.isValid()) {
    return QDateTime();
  }

  if (!msecs.isEmpty()) {
    QTime time = parsed.time();
    time = QTime(time.hour(), time.minute(), time.second(), msecs.toInt());
    parsed.setTime(time);
  }

  if (utc || zone == "+00:00" || zone == "+0000") {
    parsed.setTimeSpec(Qt::UTC);
  } else {
    parsed.setTimeSpec(Qt::LocalTime);
  }

  return parsed;
}
} // namespace

QString DiscordUser::displayName() const {
  if (!globalName.isEmpty()) {
    return globalName;
  }

  if (!username.isEmpty()) {
    return username;
  }

  return id;
}

bool DiscordAttachment::isImage() const {
  QString type = contentType.toLower();
  if (type.startsWith("image/")) {
    return true;
  }

  QString name = filename.toLower();
  return name.endsWith(".png") || name.endsWith(".jpg") ||
         name.endsWith(".jpeg") || name.endsWith(".gif") ||
         name.endsWith(".webp") || name.endsWith(".bmp");
}

QVariantMap DiscordAttachment::toVariantMap() const {
  QVariantMap data;
  data["id"] = id;
  data["filename"] = filename;
  data["url"] = url;
  data["proxyUrl"] = proxyUrl;
  data["contentType"] = contentType;
  data["size"] = size;
  data["width"] = width;
  data["height"] = height;
  data["isImage"] = isImage();
  return data;
}

DiscordAttachment DiscordAttachment::fromVariantMap(const QVariantMap &data) {
  DiscordAttachment attachment;
  attachment.id = data.value("id").toString();
  attachment.filename = data.value("filename").toString();
  attachment.url = data.value("url").toString();
  attachment.proxyUrl =
      data.value("proxy_url", data.value("proxyUrl")).toString();
  attachment.contentType =
      data.value("content_type", data.value("contentType")).toString();
  attachment.size = data.value("size").toInt();
  attachment.width = data.value("width").toInt();
  attachment.height = data.value("height").toInt();
  return attachment;
}

bool DiscordMessage::isEdited() const { return !editedTimestamp.isEmpty(); }

qint64 DiscordMessage::timestampMs() const {
  QDateTime parsed = parseDiscordTimestamp(timestamp);
  if (!parsed.isValid()) {
    return 0;
  }
  return parsed.toMSecsSinceEpoch();
}

QString DiscordMessage::displayTime() const {
  QDateTime parsed = parseDiscordTimestamp(timestamp);
  if (!parsed.isValid()) {
    return pending ? QString("Sending...") : QString();
  }

  return parsed.toLocalTime().toString("hh:mm");
}

QString DiscordMessage::authorInitials() const {
  QString name = author.displayName().trimmed();
  if (name.isEmpty()) {
    return QString();
  }

  return name.left(1).toUpper();
}

QVariantMap DiscordMessage::toVariantMap() const {
  QVariantMap data;
  data["id"] = id;
  data["channelId"] = channelId;
  data["guildId"] = guildId;
  data["authorId"] = author.id;
  data["author"] = author.displayName();
  data["username"] = author.username;
  data["avatarHash"] = author.avatarHash;
  data["initials"] = authorInitials();
  data["nonce"] = nonce;
  data["message"] = content;
  data["content"] = content;
  data["timestamp"] = timestamp;
  data["timestampMs"] = timestampMs();
  data["time"] = displayTime();
  data["editedTimestamp"] = editedTimestamp;
  data["edited"] = isEdited();
  data["replyMessageId"] = replyMessageId;
  data["replyAuthor"] = replyAuthor;
  data["replyMessage"] = replyContent;
  data["pending"] = pending;
  data["failed"] = failed;

  QVariantList attachmentList;
  for (int i = 0; i < attachments.size(); ++i) {
    attachmentList.append(attachments.at(i).toVariantMap());
  }
  data["attachments"] = attachmentList;

  if (!attachments.isEmpty() && attachments.first().isImage()) {
    const DiscordAttachment &image = attachments.first();
    data["image"] = QString();
    data["imageWidth"] = image.width;
    data["imageHeight"] = image.height;
    data["attachmentUrl"] = image.url;
    data["attachmentName"] = image.filename;
    data["attachmentIsImage"] = true;
  } else {
    data["image"] = QString();
    data["imageWidth"] = 0;
    data["imageHeight"] = 0;
    if (!attachments.isEmpty()) {
      const DiscordAttachment &attachment = attachments.first();
      data["attachmentUrl"] = attachment.url;
      data["attachmentName"] = attachment.filename;
      data["attachmentIsImage"] = false;
    } else {
      data["attachmentUrl"] = QString();
      data["attachmentName"] = QString();
      data["attachmentIsImage"] = false;
    }
  }

  return data;
}

DiscordMessage DiscordMessage::fromVariantMap(const QVariantMap &data) {
  DiscordMessage message;
  message.id = data.value("id").toString();
  message.channelId =
      data.value("channel_id", data.value("channelId")).toString();
  message.guildId = data.value("guild_id", data.value("guildId")).toString();
  message.content = data.value("content", data.value("message")).toString();
  message.nonce = data.value("nonce").toString();
  message.timestamp = data.value("timestamp").toString();
  message.editedTimestamp =
      data.value("edited_timestamp", data.value("editedTimestamp")).toString();
  message.pending = data.value("pending").toBool();
  message.failed = data.value("failed").toBool();

  QVariantMap authorData = data.value("author").toMap();
  if (!authorData.isEmpty()) {
    message.author.id = authorData.value("id").toString();
    message.author.username = authorData.value("username").toString();
    message.author.globalName =
        authorData.value("global_name", authorData.value("globalName"))
            .toString();
    message.author.discriminator = authorData.value("discriminator").toString();
    message.author.avatarHash = authorData.value("avatar").toString();
    message.author.bot = authorData.value("bot").toBool();
  } else {
    message.author.id = data.value("authorId").toString();
    message.author.username =
        data.value("username", data.value("author")).toString();
    message.author.globalName = data.value("author").toString();
    message.author.avatarHash = data.value("avatarHash").toString();
  }

  QVariantMap reference = data.value("referenced_message").toMap();
  if (!reference.isEmpty()) {
    message.replyMessageId = reference.value("id").toString();
    QVariantMap refAuthor = reference.value("author").toMap();
    DiscordUser user;
    user.id = refAuthor.value("id").toString();
    user.username = refAuthor.value("username").toString();
    user.globalName =
        refAuthor.value("global_name", refAuthor.value("globalName"))
            .toString();
    message.replyAuthor = user.displayName();
    message.replyContent = reference.value("content").toString();
  } else {
    QVariantMap messageReference = data.value("message_reference").toMap();
    message.replyMessageId =
        data.value("replyMessageId", messageReference.value("message_id"))
            .toString();
    message.replyAuthor = data.value("replyAuthor").toString();
    message.replyContent =
        data.value("replyMessage", data.value("replyContent")).toString();
  }

  QVariantList attachmentList = data.value("attachments").toList();
  for (int i = 0; i < attachmentList.size(); ++i) {
    message.attachments.append(
        DiscordAttachment::fromVariantMap(attachmentList.at(i).toMap()));
  }

  return message;
}
