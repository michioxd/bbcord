#ifndef AppStore_HPP_
#define AppStore_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>

#include "models/Models.hpp"

class AppStore : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(
      QString currentUserName READ currentUserName NOTIFY currentUserChanged)
  Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)
  Q_PROPERTY(
      QString currentUserTag READ currentUserTag NOTIFY currentUserChanged)
  Q_PROPERTY(QString currentUserAvatarSource READ currentUserAvatarSource NOTIFY
                 currentUserChanged)
  Q_PROPERTY(QVariantList guilds READ guilds NOTIFY guildsChanged)
  Q_PROPERTY(QVariantList dmChannels READ dmChannels NOTIFY dmChannelsChanged)
  Q_PROPERTY(
      QString selectedGuildId READ selectedGuildId NOTIFY selectionChanged)
  Q_PROPERTY(
      QString selectedChannelId READ selectedChannelId NOTIFY selectionChanged)

public:
  explicit AppStore(QObject *parent = 0);

  bool loggedIn() const;
  bool busy() const;
  QString statusText() const;
  QString currentUserName() const;
  QString currentUserId() const;
  QString currentUserTag() const;
  QString currentUserAvatarSource() const;
  QVariantList guilds() const;
  QVariantList dmChannels() const;
  QString selectedGuildId() const;
  QString selectedChannelId() const;

  Q_INVOKABLE void selectHome();
  Q_INVOKABLE void selectGuild(const QString &guildId);
  Q_INVOKABLE void selectChannel(const QString &channelId);
  Q_INVOKABLE void clearSession();

  void setLoggedIn(bool loggedIn);
  void setBusy(bool busy);
  void setStatusText(const QString &statusText);
  void setCurrentUser(const DiscordUser &user);
  void setCurrentUserAvatarSource(const QString &avatarSource);
  void setGuilds(const QVariantList &guilds);
  void setDmChannels(const QVariantList &dmChannels);

Q_SIGNALS:
  void loggedInChanged(bool loggedIn);
  void busyChanged(bool busy);
  void statusTextChanged(const QString &statusText);
  void currentUserChanged();
  void guildsChanged();
  void dmChannelsChanged();
  void selectionChanged();

private:
  bool m_loggedIn;
  bool m_busy;
  QString m_statusText;
  DiscordUser m_currentUser;
  QString m_currentUserAvatarSource;
  QVariantList m_guilds;
  QVariantList m_dmChannels;
  QString m_selectedGuildId;
  QString m_selectedChannelId;
};

#endif /* AppStore_HPP_ */
