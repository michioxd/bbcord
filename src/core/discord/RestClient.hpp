#ifndef RestClient_HPP_
#define RestClient_HPP_

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct mg_mgr;
struct mg_connection;

class DiscordRestClient : public QObject {
  Q_OBJECT

public:
  explicit DiscordRestClient(QObject *parent = 0);
  virtual ~DiscordRestClient();

  void loginWithToken(const QString &token);
  void fetchGuilds(const QString &token, int limit, const QString &afterId);
  void fetchDmChannels(const QString &token, int limit, const QString &afterId);
  void fetchGuildChannels(const QString &token, const QString &guildId,
                          int limit, const QString &afterId);
  void fetchChannelMessages(const QString &token, const QString &channelId,
                            int limit, const QString &beforeMessageId);
  void sendChannelMessage(const QString &token, const QString &channelId,
                          const QString &content, const QString &nonce,
                          const QString &replyMessageId,
                          const QString &attachmentPath);
  void editChannelMessage(const QString &token, const QString &channelId,
                          const QString &messageId, const QString &content);
  void deleteChannelMessage(const QString &token, const QString &channelId,
                            const QString &messageId);
  void downloadAvatar(const QString &userId, const QString &avatarHash,
                      const QString &outputPath);
  void downloadGuildIcon(const QString &guildId, const QString &iconHash,
                         const QString &outputPath);
  void cancel();

Q_SIGNALS:
  void loginSucceeded(const QVariantMap &user);
  void loginFailed(const QString &message);
  void guildsLoaded(const QVariantList &guilds);
  void dmChannelsLoaded(const QVariantList &channels);
  void guildChannelsLoaded(const QString &guildId,
                           const QVariantList &channels);
  void channelMessagesLoaded(const QString &channelId,
                             const QString &beforeMessageId,
                             const QVariantList &messages);
  void channelMessageSent(const QString &channelId, const QString &nonce,
                          const QVariantMap &message);
  void channelMessageEdited(const QString &channelId,
                            const QVariantMap &message);
  void channelMessageDeleted(const QString &channelId,
                             const QString &messageId);
  void chatRequestFailed(const QString &operation, const QString &channelId,
                         const QString &nonce, const QString &message);
  void requestFailed(const QString &message);
  void avatarDownloaded(const QString &userId, const QString &localPath);
  void avatarDownloadFailed(const QString &userId, const QString &message);
  void guildIconDownloaded(const QString &guildId, const QString &localPath);
  void guildIconDownloadFailed(const QString &guildId, const QString &message);

protected:
  virtual void timerEvent(QTimerEvent *event);

private:
  enum RequestType {
    NoRequest,
    LoginRequest,
    GuildsRequest,
    DmChannelsRequest,
    GuildChannelsRequest,
    ChannelMessagesRequest,
    SendMessageRequest,
    UploadMessageRequest,
    EditMessageRequest,
    DeleteMessageRequest,
    AvatarRequest,
    GuildIconRequest
  };

  static void eventHandler(struct mg_connection *connection, int event,
                           void *eventData);
  void handleEvent(struct mg_connection *connection, int event,
                   void *eventData);
  void startTimerIfNeeded();
  void stopTimerIfIdle();
  void finishRequest();
  void failWithMessage(const QString &message);
  void failDataRequest(const QString &message);
  void failChatRequest(const QString &message);
  void succeedWithUser(const QVariantMap &user);
  void sendGetMeRequest(struct mg_connection *connection);
  void sendApiRequest(struct mg_connection *connection);
  void sendAvatarRequest(struct mg_connection *connection);
  void sendGuildIconRequest(struct mg_connection *connection);
  QByteArray buildMultipartMessageBody(const QString &content,
                                       const QString &nonce,
                                       const QString &replyMessageId,
                                       const QString &attachmentPath,
                                       QString *contentType,
                                       QString *errorMessage) const;

  mg_mgr *m_mgr;
  mg_connection *m_connection;
  int m_timerId;
  int m_pollTicks;
  RequestType m_requestType;
  QString m_token;
  QString m_requestPath;
  QString m_requestMethod;
  QByteArray m_requestBody;
  QString m_guildId;
  QString m_channelId;
  QString m_messageId;
  QString m_beforeMessageId;
  QString m_nonce;
  QString m_avatarUserId;
  QString m_avatarHash;
  QString m_iconGuildId;
  QString m_iconHash;
  QString m_outputPath;
  QString m_contentType;
  bool m_requestSent;
  bool m_finished;
};

#endif /* RestClient_HPP_ */
