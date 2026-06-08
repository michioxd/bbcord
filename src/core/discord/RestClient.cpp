#include "RestClient.hpp"

#include "JsonParser.hpp"

#include <bb/data/JsonDataAccess>

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

extern "C" {
#include "mongoose.h"
}

#include <string.h>

namespace {
const char *kDiscordApiHost = "discord.com";
const char *kDiscordApiUrl = "https://discord.com";
const char *kDiscordCdnHost = "cdn.discordapp.com";
const char *kDiscordCdnUrl = "https://cdn.discordapp.com";
const int kPollIntervalMs = 50;
const int kRequestTimeoutTicks = 600;
const qint64 kMaxAttachmentBytes = 8 * 1024 * 1024;

QByteArray httpBodyToBytes(const struct mg_http_message *message) {
  return QByteArray(message->body.buf, static_cast<int>(message->body.len));
}

QString restErrorMessage(int status) {
  switch (status) {
  case 401:
  case 403:
    return "Invalid Discord token";
  case 429:
    return "Discord REST rate limited";
  case 0:
    return "Discord REST response is invalid";
  default:
    return QString("Discord REST error %1").arg(status);
  }
}

QString avatarErrorMessage(int status) {
  if (status == 0) {
    return "Discord avatar response is invalid";
  }

  return QString("Discord avatar error %1").arg(status);
}

QString dataErrorMessage(const QString &name, int status) {
  if (status == 401 || status == 403) {
    return "Invalid Discord token";
  }

  if (status == 429) {
    return "Discord REST rate limited";
  }

  return QString("Discord %1 error %2").arg(name).arg(status);
}

QByteArray buildJsonBody(const QVariantMap &data, QString *errorMessage) {
  bb::data::JsonDataAccess json;
  QByteArray body;
  json.saveToBuffer(data, &body);
  if (json.hasError()) {
    if (errorMessage != 0) {
      *errorMessage = json.error().errorMessage();
    }
    return QByteArray();
  }

  if (errorMessage != 0) {
    errorMessage->clear();
  }
  return body;
}
} // namespace

DiscordRestClient::DiscordRestClient(QObject *parent)
    : QObject(parent), m_connection(NULL), m_timerId(0), m_pollTicks(0),
      m_requestType(NoRequest), m_requestSent(false), m_finished(true) {
  m_mgr = new mg_mgr;
  mg_mgr_init(m_mgr);
  mg_log_set(MG_LL_NONE);
}

DiscordRestClient::~DiscordRestClient() {
  cancel();
  mg_mgr_free(m_mgr);
  delete m_mgr;
}

void DiscordRestClient::loginWithToken(const QString &token) {
  cancel();

  m_token = token.trimmed();
  if (m_token.isEmpty()) {
    emit loginFailed("Token is empty");
    return;
  }

  m_requestType = LoginRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    finishRequest();
    emit loginFailed("Could not create Discord REST connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::fetchGuilds(const QString &token, int limit,
                                    const QString &afterId) {
  cancel();

  m_token = token.trimmed();
  if (m_token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  m_requestPath = QString("/api/v9/users/@me/guilds?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    m_requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  m_requestType = GuildsRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    finishRequest();
    emit requestFailed("Could not create Discord guilds connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::fetchDmChannels(const QString &token, int limit,
                                        const QString &afterId) {
  cancel();

  m_token = token.trimmed();
  if (m_token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  m_requestPath = QString("/api/v9/users/@me/channels?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    m_requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  m_requestType = DmChannelsRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    finishRequest();
    emit requestFailed("Could not create Discord DM connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::fetchGuildChannels(const QString &token,
                                           const QString &guildId, int limit,
                                           const QString &afterId) {
  cancel();

  m_token = token.trimmed();
  m_guildId = guildId.trimmed();
  if (m_token.isEmpty() || m_guildId.isEmpty()) {
    emit requestFailed("Guild request is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  m_requestPath =
      QString("/api/v9/guilds/%1/channels?limit=%2").arg(m_guildId).arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    m_requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  m_requestType = GuildChannelsRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    finishRequest();
    emit requestFailed("Could not create Discord channel connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::fetchChannelMessages(const QString &token,
                                             const QString &channelId,
                                             int limit,
                                             const QString &beforeMessageId) {
  cancel();

  m_token = token.trimmed();
  m_channelId = channelId.trimmed();
  m_beforeMessageId = beforeMessageId.trimmed();
  if (m_token.isEmpty() || m_channelId.isEmpty()) {
    emit chatRequestFailed("messages", m_channelId, QString(),
                           "Message request is empty");
    return;
  }

  if (limit <= 0) {
    limit = 50;
  } else if (limit > 100) {
    limit = 100;
  }

  m_requestMethod = "GET";
  m_requestPath = QString("/api/v9/channels/%1/messages?limit=%2")
                      .arg(m_channelId)
                      .arg(limit);
  if (!m_beforeMessageId.isEmpty()) {
    m_requestPath += QString("&before=%1").arg(m_beforeMessageId);
  }

  m_requestType = ChannelMessagesRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString safeChannelId = m_channelId;
    finishRequest();
    emit chatRequestFailed("messages", safeChannelId, QString(),
                           "Could not create Discord messages connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::sendChannelMessage(const QString &token,
                                           const QString &channelId,
                                           const QString &content,
                                           const QString &nonce,
                                           const QString &replyMessageId,
                                           const QString &attachmentPath) {
  cancel();

  m_token = token.trimmed();
  m_channelId = channelId.trimmed();
  m_nonce = nonce.trimmed();
  QString safeContent = content.trimmed();
  QString safeAttachmentPath = attachmentPath.trimmed();
  if (m_token.isEmpty() || m_channelId.isEmpty() ||
      (safeContent.isEmpty() && safeAttachmentPath.isEmpty()) ||
      m_nonce.isEmpty()) {
    emit chatRequestFailed("send", m_channelId, m_nonce,
                           "Send message request is empty");
    return;
  }

  if (safeAttachmentPath.isEmpty()) {
    QVariantMap body;
    body["content"] = safeContent;
    body["nonce"] = m_nonce;
    body["tts"] = false;
    QString safeReplyMessageId = replyMessageId.trimmed();
    if (!safeReplyMessageId.isEmpty()) {
      QVariantMap reference;
      reference["channel_id"] = m_channelId;
      reference["message_id"] = safeReplyMessageId;
      body["message_reference"] = reference;
    }

    QString jsonError;
    m_requestBody = buildJsonBody(body, &jsonError);
    if (!jsonError.isEmpty()) {
      emit chatRequestFailed(
          "send", m_channelId, m_nonce,
          QString("Discord REST JSON error: %1").arg(jsonError));
      return;
    }

    m_contentType = "application/json";
    m_requestType = SendMessageRequest;
  } else {
    QString multipartContentType;
    QString multipartError;
    m_requestBody = buildMultipartMessageBody(
        safeContent, m_nonce, replyMessageId, safeAttachmentPath,
        &multipartContentType, &multipartError);
    if (!multipartError.isEmpty()) {
      emit chatRequestFailed("send", m_channelId, m_nonce, multipartError);
      return;
    }

    m_contentType = multipartContentType;
    m_requestType = UploadMessageRequest;
  }

  m_requestMethod = "POST";
  m_requestPath = QString("/api/v9/channels/%1/messages").arg(m_channelId);
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString safeChannelId = m_channelId;
    QString safeNonce = m_nonce;
    finishRequest();
    emit chatRequestFailed("send", safeChannelId, safeNonce,
                           "Could not create Discord send connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::editChannelMessage(const QString &token,
                                           const QString &channelId,
                                           const QString &messageId,
                                           const QString &content) {
  cancel();

  m_token = token.trimmed();
  m_channelId = channelId.trimmed();
  m_messageId = messageId.trimmed();
  QString safeContent = content.trimmed();
  if (m_token.isEmpty() || m_channelId.isEmpty() || m_messageId.isEmpty() ||
      safeContent.isEmpty()) {
    emit chatRequestFailed("edit", m_channelId, QString(),
                           "Edit message request is empty");
    return;
  }

  QVariantMap body;
  body["content"] = safeContent;
  QString jsonError;
  m_requestBody = buildJsonBody(body, &jsonError);
  if (!jsonError.isEmpty()) {
    emit chatRequestFailed(
        "edit", m_channelId, QString(),
        QString("Discord REST JSON error: %1").arg(jsonError));
    return;
  }

  m_requestMethod = "PATCH";
  m_requestPath = QString("/api/v9/channels/%1/messages/%2")
                      .arg(m_channelId)
                      .arg(m_messageId);
  m_requestType = EditMessageRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString safeChannelId = m_channelId;
    finishRequest();
    emit chatRequestFailed("edit", safeChannelId, QString(),
                           "Could not create Discord edit connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::deleteChannelMessage(const QString &token,
                                             const QString &channelId,
                                             const QString &messageId) {
  cancel();

  m_token = token.trimmed();
  m_channelId = channelId.trimmed();
  m_messageId = messageId.trimmed();
  if (m_token.isEmpty() || m_channelId.isEmpty() || m_messageId.isEmpty()) {
    emit chatRequestFailed("delete", m_channelId, QString(),
                           "Delete message request is empty");
    return;
  }

  m_requestMethod = "DELETE";
  m_requestPath = QString("/api/v9/channels/%1/messages/%2")
                      .arg(m_channelId)
                      .arg(m_messageId);
  m_requestType = DeleteMessageRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordApiUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString safeChannelId = m_channelId;
    finishRequest();
    emit chatRequestFailed("delete", safeChannelId, QString(),
                           "Could not create Discord delete connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::downloadAvatar(const QString &userId,
                                       const QString &avatarHash,
                                       const QString &outputPath) {
  cancel();

  m_avatarUserId = userId.trimmed();
  m_avatarHash = avatarHash.trimmed();
  m_outputPath = outputPath.trimmed();
  if (m_avatarUserId.isEmpty() || m_avatarHash.isEmpty() ||
      m_outputPath.isEmpty()) {
    emit avatarDownloadFailed(m_avatarUserId, "Avatar request is empty");
    return;
  }

  m_requestType = AvatarRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordCdnUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    finishRequest();
    emit avatarDownloadFailed(m_avatarUserId,
                              "Could not create Discord avatar connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::downloadGuildIcon(const QString &guildId,
                                          const QString &iconHash,
                                          const QString &outputPath) {
  cancel();

  m_iconGuildId = guildId.trimmed();
  m_iconHash = iconHash.trimmed();
  m_outputPath = outputPath.trimmed();
  if (m_iconGuildId.isEmpty() || m_iconHash.isEmpty() ||
      m_outputPath.isEmpty()) {
    emit guildIconDownloadFailed(m_iconGuildId, "Guild icon request is empty");
    return;
  }

  m_requestType = GuildIconRequest;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;
  m_connection = mg_http_connect(m_mgr, kDiscordCdnUrl,
                                 DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString safeGuildId = m_iconGuildId;
    finishRequest();
    emit guildIconDownloadFailed(safeGuildId,
                                 "Could not create Discord icon connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::cancel() {
  if (m_connection != NULL && !m_connection->is_closing) {
    m_connection->fn_data = NULL;
    m_connection->is_closing = 1;
  }
  m_connection = NULL;
  m_requestType = NoRequest;
  m_requestSent = false;
  m_finished = true;
  m_pollTicks = 0;
  m_requestPath.clear();
  m_requestMethod.clear();
  m_requestBody.clear();
  m_guildId.clear();
  m_channelId.clear();
  m_messageId.clear();
  m_beforeMessageId.clear();
  m_nonce.clear();
  m_avatarUserId.clear();
  m_avatarHash.clear();
  m_iconGuildId.clear();
  m_iconHash.clear();
  m_outputPath.clear();
  stopTimerIfIdle();
}

void DiscordRestClient::timerEvent(QTimerEvent *event) {
  Q_UNUSED(event);
  mg_mgr_poll(m_mgr, 0);

  if (m_connection != NULL && !m_finished) {
    ++m_pollTicks;
    if (m_pollTicks > kRequestTimeoutTicks) {
      failWithMessage("Discord REST timeout");
    }
  } else {
    stopTimerIfIdle();
  }
}

void DiscordRestClient::eventHandler(struct mg_connection *connection,
                                     int event, void *eventData) {
  DiscordRestClient *client =
      static_cast<DiscordRestClient *>(connection->fn_data);
  if (client != NULL) {
    client->handleEvent(connection, event, eventData);
  }
}

void DiscordRestClient::handleEvent(struct mg_connection *connection, int event,
                                    void *eventData) {
  switch (event) {
  case MG_EV_CONNECT:
    if (connection->is_tls) {
      struct mg_tls_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.name = mg_str(
          (m_requestType == AvatarRequest || m_requestType == GuildIconRequest)
              ? kDiscordCdnHost
              : kDiscordApiHost);
      opts.skip_verification = true;
      mg_tls_init(connection, &opts);
    }
    break;

  case MG_EV_HTTP_HDRS:
    break;

  case MG_EV_TLS_HS:
    if (m_requestType == AvatarRequest) {
      sendAvatarRequest(connection);
    } else if (m_requestType == GuildIconRequest) {
      sendGuildIconRequest(connection);
    } else if (m_requestType == LoginRequest) {
      sendGetMeRequest(connection);
    } else {
      sendApiRequest(connection);
    }
    break;

  case MG_EV_HTTP_MSG: {
    struct mg_http_message *message =
        static_cast<struct mg_http_message *>(eventData);
    int status = mg_http_status(message);
    QByteArray body = httpBodyToBytes(message);

    if (m_requestType == AvatarRequest || m_requestType == GuildIconRequest) {
      qDebug() << "[discord-rest] avatar status" << status;
      if (status == 200) {
        QByteArray outputPathBytes = m_outputPath.toUtf8();
        if (!mg_file_write(&mg_fs_posix, outputPathBytes.constData(),
                           body.constData(),
                           static_cast<size_t>(body.size()))) {
          QString outputPath = m_outputPath;
          QString avatarUserId = m_avatarUserId;
          QString guildId = m_iconGuildId;
          bool isGuildIcon = m_requestType == GuildIconRequest;
          finishRequest();
          if (isGuildIcon) {
            emit guildIconDownloadFailed(
                guildId, QString("Could not save icon: %1").arg(outputPath));
          } else {
            emit avatarDownloadFailed(
                avatarUserId,
                QString("Could not save avatar: %1").arg(outputPath));
          }
          break;
        }

        QString outputPath = m_outputPath;
        QString avatarUserId = m_avatarUserId;
        QString guildId = m_iconGuildId;
        bool isGuildIcon = m_requestType == GuildIconRequest;
        finishRequest();
        if (isGuildIcon) {
          emit guildIconDownloaded(guildId, outputPath);
        } else {
          emit avatarDownloaded(avatarUserId, outputPath);
        }
        break;
      }

      QString errorMessage = avatarErrorMessage(status);
      QString avatarUserId = m_avatarUserId;
      QString guildId = m_iconGuildId;
      bool isGuildIcon = m_requestType == GuildIconRequest;
      finishRequest();
      if (isGuildIcon) {
        emit guildIconDownloadFailed(guildId, errorMessage);
      } else {
        emit avatarDownloadFailed(avatarUserId, errorMessage);
      }
      break;
    }

    if (m_requestType == ChannelMessagesRequest) {
      qDebug() << "[discord-rest] channel messages status" << status;
      if (status == 200) {
        QString parseError;
        QVariantList messages =
            DiscordJsonParser::parseArray(body, &parseError);
        if (!parseError.isEmpty()) {
          failChatRequest(
              QString("Discord REST JSON error: %1").arg(parseError));
          break;
        }

        QString channelId = m_channelId;
        QString beforeMessageId = m_beforeMessageId;
        finishRequest();
        emit channelMessagesLoaded(channelId, beforeMessageId, messages);
        break;
      }

      failChatRequest(dataErrorMessage("messages", status));
      break;
    }

    if (m_requestType == SendMessageRequest ||
        m_requestType == UploadMessageRequest ||
        m_requestType == EditMessageRequest) {
      bool sending = m_requestType == SendMessageRequest ||
                     m_requestType == UploadMessageRequest;
      qDebug() << "[discord-rest]" << (sending ? "send" : "edit") << "status"
               << status;
      if ((sending && status == 200) || (!sending && status == 200)) {
        QString parseError;
        QVariantMap message = DiscordJsonParser::parseObject(body, &parseError);
        if (!parseError.isEmpty()) {
          failChatRequest(
              QString("Discord REST JSON error: %1").arg(parseError));
          break;
        }

        QString channelId = m_channelId;
        QString nonce = m_nonce;
        RequestType finishedType = m_requestType;
        finishRequest();
        if (finishedType == SendMessageRequest ||
            finishedType == UploadMessageRequest) {
          emit channelMessageSent(channelId, nonce, message);
        } else {
          emit channelMessageEdited(channelId, message);
        }
        break;
      }

      failChatRequest(
          dataErrorMessage(sending ? "send message" : "edit message", status));
      break;
    }

    if (m_requestType == DeleteMessageRequest) {
      qDebug() << "[discord-rest] delete status" << status;
      if (status == 204 || status == 200) {
        QString channelId = m_channelId;
        QString messageId = m_messageId;
        finishRequest();
        emit channelMessageDeleted(channelId, messageId);
        break;
      }

      failChatRequest(dataErrorMessage("delete message", status));
      break;
    }

    if (m_requestType == GuildsRequest || m_requestType == DmChannelsRequest ||
        m_requestType == GuildChannelsRequest) {
      QString requestName =
          m_requestType == GuildsRequest
              ? "guilds"
              : (m_requestType == DmChannelsRequest ? "DM channels"
                                                    : "guild channels");
      qDebug() << "[discord-rest]" << requestName << "status" << status;
      if (status == 200) {
        QString parseError;
        QVariantList items = DiscordJsonParser::parseArray(body, &parseError);
        if (!parseError.isEmpty()) {
          failDataRequest(
              QString("Discord REST JSON error: %1").arg(parseError));
          break;
        }

        QString guildId = m_guildId;
        RequestType finishedType = m_requestType;
        finishRequest();
        if (finishedType == GuildsRequest) {
          emit guildsLoaded(items);
        } else if (finishedType == DmChannelsRequest) {
          emit dmChannelsLoaded(items);
        } else {
          emit guildChannelsLoaded(guildId, items);
        }
        break;
      }

      failDataRequest(dataErrorMessage(requestName, status));
      break;
    }

    qDebug() << "[discord-rest] /users/@me status" << status;

    if (status == 200) {
      QString parseError;
      QVariantMap user = DiscordJsonParser::parseObject(body, &parseError);
      if (!parseError.isEmpty()) {
        failWithMessage(QString("Discord REST JSON error: %1").arg(parseError));
        break;
      }

      succeedWithUser(user);
      break;
    }

    failWithMessage(restErrorMessage(status));
    break;
  }

  case MG_EV_ERROR: {
    QString message = QString::fromUtf8(
        eventData ? static_cast<char *>(eventData) : "Discord REST error");
    failWithMessage(message);
    break;
  }

  case MG_EV_CLOSE:
    if (m_connection == connection) {
      failWithMessage("Discord REST connection closed");
    }
    break;
  }
}

void DiscordRestClient::startTimerIfNeeded() {
  if (m_timerId == 0) {
    m_timerId = startTimer(kPollIntervalMs);
  }
}

void DiscordRestClient::stopTimerIfIdle() {
  if (m_timerId != 0 && m_connection == NULL) {
    killTimer(m_timerId);
    m_timerId = 0;
  }
}

void DiscordRestClient::finishRequest() {
  if (m_connection != NULL) {
    m_connection->is_closing = 1;
  }
  m_connection = NULL;
  m_requestType = NoRequest;
  m_requestSent = false;
  m_finished = true;
  m_pollTicks = 0;
  m_requestPath.clear();
  m_requestMethod.clear();
  m_requestBody.clear();
  m_contentType.clear();
  m_guildId.clear();
  m_channelId.clear();
  m_messageId.clear();
  m_beforeMessageId.clear();
  m_nonce.clear();
  m_avatarUserId.clear();
  m_avatarHash.clear();
  m_iconGuildId.clear();
  m_iconHash.clear();
  m_outputPath.clear();
  stopTimerIfIdle();
}

void DiscordRestClient::failWithMessage(const QString &message) {
  if (m_finished) {
    return;
  }

  QString safeMessage = message.trimmed();
  if (safeMessage.isEmpty()) {
    safeMessage = "Discord REST error";
  }

  finishRequest();
  emit loginFailed(safeMessage);
}

void DiscordRestClient::failDataRequest(const QString &message) {
  if (m_finished) {
    return;
  }

  QString safeMessage = message.trimmed();
  if (safeMessage.isEmpty()) {
    safeMessage = "Discord REST error";
  }

  finishRequest();
  emit requestFailed(safeMessage);
}

void DiscordRestClient::failChatRequest(const QString &message) {
  if (m_finished) {
    return;
  }

  QString safeMessage = message.trimmed();
  if (safeMessage.isEmpty()) {
    safeMessage = "Discord REST error";
  }

  QString operation;
  if (m_requestType == ChannelMessagesRequest) {
    operation = "messages";
  } else if (m_requestType == SendMessageRequest ||
             m_requestType == UploadMessageRequest) {
    operation = "send";
  } else if (m_requestType == EditMessageRequest) {
    operation = "edit";
  } else if (m_requestType == DeleteMessageRequest) {
    operation = "delete";
  }

  QString channelId = m_channelId;
  QString nonce = m_nonce;
  finishRequest();
  emit chatRequestFailed(operation, channelId, nonce, safeMessage);
}

QByteArray DiscordRestClient::buildMultipartMessageBody(
    const QString &content, const QString &nonce, const QString &replyMessageId,
    const QString &attachmentPath, QString *contentType,
    QString *errorMessage) const {
  QFile file(attachmentPath);
  if (!file.open(QIODevice::ReadOnly)) {
    if (errorMessage != 0) {
      *errorMessage = QString("Could not open attachment: %1")
                          .arg(QFileInfo(attachmentPath).fileName());
    }
    return QByteArray();
  }

  if (file.size() > kMaxAttachmentBytes) {
    if (errorMessage != 0) {
      *errorMessage = QString("Attachment is too large: %1")
                          .arg(QFileInfo(attachmentPath).fileName());
    }
    return QByteArray();
  }

  QByteArray fileBytes = file.readAll();
  QFileInfo fileInfo(attachmentPath);
  QVariantMap payload;
  payload["content"] = content;
  payload["nonce"] = nonce;
  payload["tts"] = false;
  QString safeReplyMessageId = replyMessageId.trimmed();
  if (!safeReplyMessageId.isEmpty()) {
    QVariantMap reference;
    reference["channel_id"] = m_channelId;
    reference["message_id"] = safeReplyMessageId;
    payload["message_reference"] = reference;
  }

  QVariantList attachments;
  QVariantMap attachment;
  attachment["id"] = "0";
  attachment["filename"] = fileInfo.fileName();
  attachments.append(attachment);
  payload["attachments"] = attachments;

  QString jsonError;
  QByteArray payloadBytes = buildJsonBody(payload, &jsonError);
  if (!jsonError.isEmpty()) {
    if (errorMessage != 0) {
      *errorMessage = QString("Discord REST JSON error: %1").arg(jsonError);
    }
    return QByteArray();
  }

  QByteArray boundary =
      QString("----BBCord%1").arg(QDateTime::currentMSecsSinceEpoch()).toUtf8();
  QByteArray body;
  body.append("--" + boundary + "\r\n");
  body.append("Content-Disposition: form-data; name=\"payload_json\"\r\n");
  body.append("Content-Type: application/json\r\n\r\n");
  body.append(payloadBytes);
  body.append("\r\n--" + boundary + "\r\n");
  body.append("Content-Disposition: form-data; name=\"files[0]\"; filename=\"");
  body.append(fileInfo.fileName().toUtf8());
  body.append("\"\r\n");
  body.append("Content-Type: application/octet-stream\r\n\r\n");
  body.append(fileBytes);
  body.append("\r\n--" + boundary + "--\r\n");

  if (contentType != 0) {
    *contentType = QString("multipart/form-data; boundary=%1")
                       .arg(QString::fromUtf8(boundary));
  }
  if (errorMessage != 0) {
    errorMessage->clear();
  }
  return body;
}

void DiscordRestClient::succeedWithUser(const QVariantMap &user) {
  if (m_finished) {
    return;
  }

  finishRequest();
  emit loginSucceeded(user);
}
