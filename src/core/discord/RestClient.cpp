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
const int kPollIntervalMs = 50;
const int kRequestTimeoutTicks = 600;

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

void DiscordRestClient::downloadAvatar(const QString &userId,
                                       const QString &avatarHash,
                                       const QString &outputPath) {
  cancel();

  m_avatarUserId = userId.trimmed();
  m_avatarHash = avatarHash.trimmed();
  m_outputPath = outputPath.trimmed();
  if (m_avatarUserId.isEmpty() || m_avatarHash.isEmpty() ||
      m_outputPath.isEmpty()) {
    emit avatarDownloadFailed("Avatar request is empty");
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
    emit avatarDownloadFailed("Could not create Discord avatar connection");
    return;
  }

  startTimerIfNeeded();
}

void DiscordRestClient::cancel() {
  if (m_connection != NULL && !m_connection->is_closing) {
    m_connection->is_closing = 1;
  }
  m_connection = NULL;
  m_requestType = NoRequest;
  m_requestSent = false;
  m_finished = true;
  m_pollTicks = 0;
  m_avatarUserId.clear();
  m_avatarHash.clear();
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
      opts.name = mg_str(m_requestType == AvatarRequest ? kDiscordCdnHost
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
    } else {
      sendGetMeRequest(connection);
    }
    break;

  case MG_EV_HTTP_MSG: {
    struct mg_http_message *message =
        static_cast<struct mg_http_message *>(eventData);
    int status = mg_http_status(message);
    QByteArray body = httpBodyToBytes(message);

    if (m_requestType == AvatarRequest) {
      qDebug() << "[discord-rest] avatar status" << status;
      if (status == 200) {
        QByteArray outputPathBytes = m_outputPath.toUtf8();
        if (!mg_file_write(&mg_fs_posix, outputPathBytes.constData(),
                           body.constData(),
                           static_cast<size_t>(body.size()))) {
          QString outputPath = m_outputPath;
          finishRequest();
          emit avatarDownloadFailed(
              QString("Could not save avatar: %1").arg(outputPath));
          break;
        }

        QString outputPath = m_outputPath;
        finishRequest();
        emit avatarDownloaded(outputPath);
        break;
      }

      QString errorMessage = avatarErrorMessage(status);
      finishRequest();
      emit avatarDownloadFailed(errorMessage);
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
  m_avatarUserId.clear();
  m_avatarHash.clear();
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

void DiscordRestClient::succeedWithUser(const QVariantMap &user) {
  if (m_finished) {
    return;
  }

  finishRequest();
  emit loginSucceeded(user);
}

void DiscordRestClient::sendGetMeRequest(struct mg_connection *connection) {
  if (m_requestType != LoginRequest || connection == NULL || m_requestSent) {
    return;
  }

  m_requestSent = true;

  QByteArray tokenBytes = m_token.toUtf8();
  mg_printf(connection,
            "GET /api/v9/users/@me HTTP/1.1\r\n"
            "Host: discord.com\r\n"
            "Authorization: %s\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/149.0.0.0 "
            "Safari/537.36\r\n"
            "Accept: application/json\r\n"
            "Connection: close\r\n\r\n",
            tokenBytes.constData());
}

void DiscordRestClient::sendAvatarRequest(struct mg_connection *connection) {
  if (m_requestType != AvatarRequest || connection == NULL || m_requestSent) {
    return;
  }

  m_requestSent = true;

  QByteArray userIdBytes = m_avatarUserId.toUtf8();
  QByteArray avatarHashBytes = m_avatarHash.toUtf8();
  mg_printf(connection,
            "GET /avatars/%s/%s.png?size=128 HTTP/1.1\r\n"
            "Host: cdn.discordapp.com\r\n"
            "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/149.0.0.0 "
            "Safari/537.36\r\n"
            "Accept: image/png,image/*\r\n"
            "Connection: close\r\n\r\n",
            userIdBytes.constData(), avatarHashBytes.constData());
}
