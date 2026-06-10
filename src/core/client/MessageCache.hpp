#ifndef MessageCache_HPP_
#define MessageCache_HPP_

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "../models/Models.hpp"

class MessageCache {
public:
  struct ChannelState {
    ChannelState();

    QString guildId;
    QStringList orderedIds;
    QHash<QString, DiscordMessage> messagesById;
    QSet<QString> pendingNonces;
    QString oldestMessageId;
    QString newestMessageId;
    bool initialLoaded;
    bool hasMoreBefore;
    bool loadingInitial;
    bool loadingBefore;
  };

  explicit MessageCache(int maxMessagesPerChannel = 200);

  bool hasChannel(const QString &channelId) const;
  bool isInitialLoaded(const QString &channelId) const;
  bool isLoadingInitial(const QString &channelId) const;
  bool isLoadingBefore(const QString &channelId) const;
  bool hasMoreBefore(const QString &channelId) const;
  QStringList initialLoadedChannelIds() const;
  QString oldestMessageId(const QString &channelId) const;
  QString newestMessageId(const QString &channelId) const;

  void setLoadingInitial(const QString &channelId, bool loading);
  void setLoadingBefore(const QString &channelId, bool loading);
  void setHasMoreBefore(const QString &channelId, bool hasMore);
  void setChannelGuild(const QString &channelId, const QString &guildId);

  QVariantList messagesForChannel(const QString &channelId) const;
  bool containsMessage(const QString &channelId,
                       const QString &messageId) const;
  DiscordMessage message(const QString &channelId,
                         const QString &messageId) const;

  void setInitialMessages(const QString &channelId, const QString &guildId,
                          const QList<DiscordMessage> &messages,
                          bool hasMoreBefore);
  QVariantList prependOlderMessages(const QString &channelId,
                                    const QList<DiscordMessage> &messages,
                                    bool hasMoreBefore);
  QVariantMap addOrReplaceMessage(const DiscordMessage &message);
  QVariantMap addOrReplaceMessages(const QList<DiscordMessage> &messages);
  QVariantMap updateMessage(const DiscordMessage &message);
  bool deleteMessage(const QString &channelId, const QString &messageId);
  QString addPendingMessage(const DiscordMessage &message);
  void markPendingFailed(const QString &channelId, const QString &messageId);
  void clear();

private:
  ChannelState &ensureChannel(const QString &channelId);
  const ChannelState *channel(const QString &channelId) const;
  void insertSorted(ChannelState &state, const DiscordMessage &message,
                    bool updateGrouping = true);
  void trimChannel(ChannelState &state, bool updateGrouping = true);
  void trimChannelFromBack(ChannelState &state, bool updateGrouping = true);
  void recalculateGrouping(ChannelState &state);
  int compareMessageIds(const QString &left, const QString &right) const;
  void refreshBounds(ChannelState &state);

  QHash<QString, ChannelState> m_channels;
  int m_maxMessagesPerChannel;
};

#endif /* MessageCache_HPP_ */
