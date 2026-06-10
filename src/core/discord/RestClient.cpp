#include "RestClient.hpp"

#include "JsonParser.hpp"

#include <QByteArray>
#include <QDebug>

extern "C" {
#include "mongoose.h"
}

#include <string.h>

namespace {
const char *kDiscordApiHost = "discord.com";
const char *kDiscordApiUrl = "https://discord.com";
const char *kDiscordCdnHost = "cdn.discordapp.com";
const char *kDiscordCdnUrl = "https://cdn.discordapp.com";
const int kPollIntervalMs = 10;
const int kRequestTimeoutTicks = 600;
const int kKeepAliveIdleTimeoutTicks = 300;

QByteArray httpBodyToBytes(const struct mg_http_message *message) {
  return QByteArray(message->body.buf, static_cast<int>(message->body.len));
}

bool responseAllowsKeepAlive(struct mg_http_message *message) {
  struct mg_str closeValue = mg_str("close");
  struct mg_str *connection = mg_http_get_header(message, "Connection");
  return connection == NULL || mg_strcasecmp(*connection, closeValue) != 0;
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

} // namespace

DiscordRestClient::DiscordRestClient(QObject *parent)
    : QObject(parent), m_connection(NULL), m_timerId(0), m_pollTicks(0),
      m_idleTicks(0), m_requestType(NoRequest), m_isProcessing(false),
      m_requestSent(false), m_finished(true) {
  m_mgr = new mg_mgr;
  mg_mgr_init(m_mgr);
  mg_log_set(MG_LL_NONE);
}

DiscordRestClient::~DiscordRestClient() {
  cancel();
  mg_mgr_free(m_mgr);
  delete m_mgr;
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
  m_idleTicks = 0;
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

  if (m_connection == NULL) {
    stopTimerIfIdle();
    return;
  }

  mg_mgr_poll(m_mgr, 0);

  if (m_connection != NULL && m_finished) {
    ++m_idleTicks;
    if (m_idleTicks > kKeepAliveIdleTimeoutTicks) {
      m_connection->fn_data = NULL;
      if (!m_connection->is_closing) {
        m_connection->is_closing = 1;
      }
      m_connection = NULL;
      stopTimerIfIdle();
    }
  } else if (m_connection != NULL && !m_finished) {
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

  bool reuseConnection = m_connection != NULL && !m_connection->is_closing;
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
  m_idleTicks = 0;

  if (reuseConnection) {
    sendCurrentRequest(m_connection);
    startTimerIfNeeded();
    return;
  }

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

void DiscordRestClient::sendCurrentRequest(struct mg_connection *connection) {
  if (m_requestType == AvatarRequest) {
    sendAvatarRequest(connection);
  } else if (m_requestType == GuildIconRequest) {
    sendGuildIconRequest(connection);
  } else if (m_requestType == LoginRequest) {
    sendGetMeRequest(connection);
  } else {
    sendApiRequest(connection);
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
    sendCurrentRequest(connection);
    break;

  case MG_EV_HTTP_MSG: {
    struct mg_http_message *message =
        static_cast<struct mg_http_message *>(eventData);
    int status = mg_http_status(message);
    QByteArray body = httpBodyToBytes(message);
    bool keepConnectionAlive = responseAllowsKeepAlive(message);

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
          finishRequest(keepConnectionAlive);
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
        finishRequest(keepConnectionAlive);
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
      finishRequest(keepConnectionAlive);
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
        finishRequest(keepConnectionAlive);
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
        finishRequest(keepConnectionAlive);
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
        finishRequest(keepConnectionAlive);
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
        finishRequest(keepConnectionAlive);
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
      m_connection = NULL;
      if (!m_finished) {
        failWithMessage("Discord REST connection closed");
      } else {
        stopTimerIfIdle();
      }
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

void DiscordRestClient::finishRequest(bool keepConnectionAlive) {
  if (m_connection != NULL) {
    if (keepConnectionAlive && !m_connection->is_closing) {
      m_idleTicks = 0;
    } else {
      m_connection->fn_data = NULL;
      if (!m_connection->is_closing) {
        m_connection->is_closing = 1;
      }
      m_connection = NULL;
    }
  }
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
  if (keepConnectionAlive && m_connection != NULL) {
    startTimerIfNeeded();
  }
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
