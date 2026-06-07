#ifndef Models_HPP_
#define Models_HPP_

#include <QString>

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
  QString timestamp;
  QString editedTimestamp;

  bool isEdited() const;
};

#endif /* Models_HPP_ */
