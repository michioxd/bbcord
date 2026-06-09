#ifndef Models_HPP_
#define Models_HPP_

#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct DiscordAttachment {
  QString id;
  QString filename;
  QString url;
  QString proxyUrl;
  QString contentType;
  int size;
  int width;
  int height;

  DiscordAttachment() : size(0), width(0), height(0) {}

  bool isImage() const;
  QVariantMap toVariantMap() const;
  static DiscordAttachment fromVariantMap(const QVariantMap &data);
};

struct DiscordUser {
  QString id;
  QString username;
  QString discriminator;
  QString globalName;
  QString avatarHash;
  bool bot;

  DiscordUser() : bot(false) {}

  QString displayName() const;
};

struct DiscordGuild {
  QString id;
  QString name;
  QString iconHash;
  bool unavailable;

  DiscordGuild() : unavailable(false) {}
};

struct DiscordChannel {
  enum ChannelType {
    GuildText = 0,
    Dm = 1,
    GuildVoice = 2,
    GroupDm = 3,
    GuildCategory = 4,
    GuildAnnouncement = 5,
    Unknown = -1
  };

  QString id;
  QString guildId;
  QString name;
  ChannelType type;
  int position;

  DiscordChannel() : type(Unknown), position(0) {}
};

struct DiscordMessage {
  QString id;
  QString channelId;
  QString guildId;
  DiscordUser author;
  QString content;
  QString nonce;
  QString timestamp;
  QString editedTimestamp;
  QString replyMessageId;
  QString replyAuthor;
  QString replyContent;
  QList<DiscordAttachment> attachments;
  bool pending;
  bool failed;
  bool isGroupStart;
  bool isGroupEnd;
  bool showAvatar;
  bool showUsername;
  bool showTimestamp;

  DiscordMessage()
      : pending(false), failed(false), isGroupStart(true), isGroupEnd(true),
        showAvatar(true), showUsername(true), showTimestamp(true) {}

  bool isEdited() const;
  qint64 timestampMs() const;
  QString displayTime() const;
  QString authorInitials() const;
  QVariantMap toVariantMap() const;
  static DiscordMessage fromVariantMap(const QVariantMap &data);
};

#endif /* Models_HPP_ */
