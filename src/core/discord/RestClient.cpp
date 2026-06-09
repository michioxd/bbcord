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
      m_requestType(NoRequest), m_isProcessing(false), m_requestSent(false),
      m_finished(true) {
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
  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit loginFailed("Token is empty");
    return;
  }

  request.type = LoginRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchGuilds(const QString &token, int limit,
                                    const QString &afterId) {
  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  request.requestPath = QString("/api/v9/users/@me/guilds?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = GuildsRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchDmChannels(const QString &token, int limit,
                                        const QString &afterId) {
  RestRequest request;
  request.token = token.trimmed();
  if (request.token.isEmpty()) {
    emit requestFailed("Token is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  request.requestPath =
      QString("/api/v9/users/@me/channels?limit=%1").arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = DmChannelsRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchGuildChannels(const QString &token,
                                           const QString &guildId, int limit,
                                           const QString &afterId) {
  RestRequest request;
  request.token = token.trimmed();
  request.guildId = guildId.trimmed();
  if (request.token.isEmpty() || request.guildId.isEmpty()) {
    emit requestFailed("Guild request is empty");
    return;
  }

  if (limit <= 0) {
    limit = 25;
  }

  request.requestPath = QString("/api/v9/guilds/%1/channels?limit=%2")
                            .arg(request.guildId)
                            .arg(limit);
  if (!afterId.trimmed().isEmpty()) {
    request.requestPath += QString("&after=%1").arg(afterId.trimmed());
  }

  request.type = GuildChannelsRequest;
  enqueueRequest(request);
}

void DiscordRestClient::fetchChannelMessages(const QString &token,
                                             const QString &channelId,
                                             int limit,
                                             const QString &beforeMessageId) {
  RestRequest restRequest;
  restRequest.token = token.trimmed();
  restRequest.channelId = channelId.trimmed();
  restRequest.beforeMessageId = beforeMessageId.trimmed();
  if (restRequest.token.isEmpty() || restRequest.channelId.isEmpty()) {
    emit chatRequestFailed("messages", restRequest.channelId, QString(),
                           "Message request is empty");
    return;
  }

  if (limit <= 0) {
    limit = 50;
  } else if (limit > 100) {
    limit = 100;
  }

  restRequest.requestMethod = "GET";
  restRequest.requestPath = QString("/api/v9/channels/%1/messages?limit=%2")
                                .arg(restRequest.channelId)
                                .arg(limit);
  if (!restRequest.beforeMessageId.isEmpty()) {
    restRequest.requestPath +=
        QString("&before=%1").arg(restRequest.beforeMessageId);
  }

  restRequest.type = ChannelMessagesRequest;
  enqueueRequest(restRequest);
}

void DiscordRestClient::sendChannelMessage(const QString &token,
                                           const QString &channelId,
                                           const QString &content,
                                           const QString &nonce,
                                           const QString &replyMessageId,
                                           const QString &attachmentPath) {
  RestRequest restRequest;
  restRequest.token = token.trimmed();
  restRequest.channelId = channelId.trimmed();
  restRequest.nonce = nonce.trimmed();
  QString safeContent = content.trimmed();
  QString safeAttachmentPath = attachmentPath.trimmed();
  if (restRequest.token.isEmpty() || restRequest.channelId.isEmpty() ||
      (safeContent.isEmpty() && safeAttachmentPath.isEmpty()) ||
      restRequest.nonce.isEmpty()) {
    emit chatRequestFailed("send", restRequest.channelId, restRequest.nonce,
                           "Send message request is empty");
    return;
  }

  if (safeAttachmentPath.isEmpty()) {
    QVariantMap body;
    body["content"] = safeContent;
    body["nonce"] = restRequest.nonce;
    body["tts"] = false;
    QString safeReplyMessageId = replyMessageId.trimmed();
    if (!safeReplyMessageId.isEmpty()) {
      QVariantMap reference;
      reference["channel_id"] = restRequest.channelId;
      reference["message_id"] = safeReplyMessageId;
      body["message_reference"] = reference;
    }

    QString jsonError;
    QByteArray jsonBody = buildJsonBody(body, &jsonError);
    restRequest.requestBody = jsonBody;
    if (!jsonError.isEmpty()) {
      emit chatRequestFailed(
          "send", restRequest.channelId, restRequest.nonce,
          QString("Discord REST JSON error: %1").arg(jsonError));
      return;
    }

    restRequest.contentType = "application/json";
    restRequest.type = SendMessageRequest;
  } else {
    QString multipartContentType;
    QString multipartError;
    restRequest.requestBody = buildMultipartMessageBody(
        safeContent, restRequest.nonce, restRequest.channelId, replyMessageId,
        safeAttachmentPath, &multipartContentType, &multipartError);
    if (!multipartError.isEmpty()) {
      emit chatRequestFailed("send", restRequest.channelId, restRequest.nonce,
                             multipartError);
      return;
    }

    restRequest.contentType = multipartContentType;
    restRequest.type = UploadMessageRequest;
  }

  restRequest.requestMethod = "POST";
  restRequest.requestPath =
      QString("/api/v9/channels/%1/messages").arg(restRequest.channelId);
  enqueueRequest(restRequest);
}

void DiscordRestClient::editChannelMessage(const QString &token,
                                           const QString &channelId,
                                           const QString &messageId,
                                           const QString &content) {
  RestRequest restRequest;
  restRequest.token = token.trimmed();
  restRequest.channelId = channelId.trimmed();
  restRequest.messageId = messageId.trimmed();
  QString safeContent = content.trimmed();
  if (restRequest.token.isEmpty() || restRequest.channelId.isEmpty() ||
      restRequest.messageId.isEmpty() || safeContent.isEmpty()) {
    emit chatRequestFailed("edit", restRequest.channelId, QString(),
                           "Edit message request is empty");
    return;
  }

  QVariantMap body;
  body["content"] = safeContent;
  QString jsonError;
  QByteArray jsonBody = buildJsonBody(body, &jsonError);
  restRequest.requestBody = jsonBody;
  if (!jsonError.isEmpty()) {
    emit chatRequestFailed(
        "edit", restRequest.channelId, QString(),
        QString("Discord REST JSON error: %1").arg(jsonError));
    return;
  }

  restRequest.requestMethod = "PATCH";
  restRequest.requestPath = QString("/api/v9/channels/%1/messages/%2")
                                .arg(restRequest.channelId)
                                .arg(restRequest.messageId);
  restRequest.contentType = "application/json";
  restRequest.type = EditMessageRequest;
  enqueueRequest(restRequest);
}

void DiscordRestClient::deleteChannelMessage(const QString &token,
                                             const QString &channelId,
                                             const QString &messageId) {
  RestRequest request;
  request.token = token.trimmed();
  request.channelId = channelId.trimmed();
  request.messageId = messageId.trimmed();
  if (request.token.isEmpty() || request.channelId.isEmpty() ||
      request.messageId.isEmpty()) {
    emit chatRequestFailed("delete", request.channelId, QString(),
                           "Delete message request is empty");
    return;
  }

  request.requestMethod = "DELETE";
  request.requestPath = QString("/api/v9/channels/%1/messages/%2")
                            .arg(request.channelId)
                            .arg(request.messageId);
  request.type = DeleteMessageRequest;
  enqueueRequest(request);
}

void DiscordRestClient::downloadAvatar(const QString &userId,
                                       const QString &avatarHash,
                                       const QString &outputPath) {
  RestRequest request;
  request.avatarUserId = userId.trimmed();
  request.avatarHash = avatarHash.trimmed();
  request.outputPath = outputPath.trimmed();
  if (request.avatarUserId.isEmpty() || request.avatarHash.isEmpty() ||
      request.outputPath.isEmpty()) {
    emit avatarDownloadFailed(request.avatarUserId, "Avatar request is empty");
    return;
  }

  request.type = AvatarRequest;
  enqueueRequest(request);
}

void DiscordRestClient::downloadGuildIcon(const QString &guildId,
                                          const QString &iconHash,
                                          const QString &outputPath) {
  RestRequest request;
  request.iconGuildId = guildId.trimmed();
  request.iconHash = iconHash.trimmed();
  request.outputPath = outputPath.trimmed();
  if (request.iconGuildId.isEmpty() || request.iconHash.isEmpty() ||
      request.outputPath.isEmpty()) {
    emit guildIconDownloadFailed(request.iconGuildId,
                                 "Guild icon request is empty");
    return;
  }

  request.type = GuildIconRequest;
  enqueueRequest(request);
}

void DiscordRestClient::cancel() {
  m_requestQueue.clear();
  if (m_connection != NULL) {
    m_connection->fn_data = NULL;
    if (!m_connection->is_closing) {
      m_connection->is_closing = 1;
    }
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
  m_isProcessing = false;
  stopTimerIfIdle();
}

void DiscordRestClient::timerEvent(QTimerEvent *event) {
  Q_UNUSED(event);

  if (m_connection == NULL || m_finished) {
    stopTimerIfIdle();
    return;
  }

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

void DiscordRestClient::enqueueRequest(const RestRequest &request) {
  m_requestQueue.append(request);
  processNextRequest();
}

void DiscordRestClient::processNextRequest() {
  if (m_isProcessing || m_requestQueue.isEmpty()) {
    return;
  }

  RestRequest request = m_requestQueue.takeFirst();
  m_isProcessing = true;
  m_requestType = request.type;
  m_token = request.token;
  m_requestPath = request.requestPath;
  m_requestMethod = request.requestMethod;
  m_requestBody = request.requestBody;
  m_contentType = request.contentType;
  m_guildId = request.guildId;
  m_channelId = request.channelId;
  m_messageId = request.messageId;
  m_beforeMessageId = request.beforeMessageId;
  m_nonce = request.nonce;
  m_avatarUserId = request.avatarUserId;
  m_avatarHash = request.avatarHash;
  m_iconGuildId = request.iconGuildId;
  m_iconHash = request.iconHash;
  m_outputPath = request.outputPath;
  m_requestSent = false;
  m_finished = false;
  m_pollTicks = 0;

  const char *url =
      (m_requestType == AvatarRequest || m_requestType == GuildIconRequest)
          ? kDiscordCdnUrl
          : kDiscordApiUrl;
  m_connection =
      mg_http_connect(m_mgr, url, DiscordRestClient::eventHandler, this);

  if (m_connection == NULL) {
    QString message;
    if (m_requestType == LoginRequest) {
      message = "Could not create Discord REST connection";
    } else if (m_requestType == GuildsRequest) {
      message = "Could not create Discord guilds connection";
    } else if (m_requestType == DmChannelsRequest) {
      message = "Could not create Discord DM connection";
    } else if (m_requestType == GuildChannelsRequest) {
      message = "Could not create Discord channel connection";
    } else if (m_requestType == ChannelMessagesRequest) {
      message = "Could not create Discord messages connection";
    } else if (m_requestType == SendMessageRequest ||
               m_requestType == UploadMessageRequest) {
      message = "Could not create Discord send connection";
    } else if (m_requestType == EditMessageRequest) {
      message = "Could not create Discord edit connection";
    } else if (m_requestType == DeleteMessageRequest) {
      message = "Could not create Discord delete connection";
    } else if (m_requestType == AvatarRequest) {
      message = "Could not create Discord avatar connection";
    } else if (m_requestType == GuildIconRequest) {
      message = "Could not create Discord icon connection";
    } else {
      message = "Could not create Discord REST connection";
    }

    if (m_requestType == LoginRequest) {
      failWithMessage(message);
    } else if (m_requestType == ChannelMessagesRequest ||
               m_requestType == SendMessageRequest ||
               m_requestType == UploadMessageRequest ||
               m_requestType == EditMessageRequest ||
               m_requestType == DeleteMessageRequest) {
      failChatRequest(message);
    } else if (m_requestType == AvatarRequest) {
      QString avatarUserId = m_avatarUserId;
      finishRequest();
      emit avatarDownloadFailed(avatarUserId, message);
      processNextRequest();
    } else if (m_requestType == GuildIconRequest) {
      QString guildId = m_iconGuildId;
      finishRequest();
      emit guildIconDownloadFailed(guildId, message);
      processNextRequest();
    } else {
      failDataRequest(message);
    }
    return;
  }

  startTimerIfNeeded();
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
          processNextRequest();
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
        processNextRequest();
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
      processNextRequest();
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
        processNextRequest();
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
        processNextRequest();
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
        processNextRequest();
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
        processNextRequest();
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
  if (m_timerId != 0 && (m_connection == NULL || m_finished)) {
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
  m_isProcessing = false;
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
  processNextRequest();
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
  processNextRequest();
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
  processNextRequest();
}

QByteArray DiscordRestClient::buildMultipartMessageBody(
    const QString &content, const QString &nonce, const QString &channelId,
    const QString &replyMessageId, const QString &attachmentPath,
    QString *contentType, QString *errorMessage) const {
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
    reference["channel_id"] = channelId;
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
  processNextRequest();
}
