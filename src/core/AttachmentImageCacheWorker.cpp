#include "AttachmentImageCacheWorker.hpp"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

extern "C" {
#include "mongoose.h"
}

#include <string.h>

namespace {
const int kPreviewWidth = 240;
const int kPreviewQuality = 55;
const int kPollIntervalMs = 50;
const int kRequestTimeoutTicks = 300;

struct ImageDownloadContext {
  QString host;
  QString path;
  QByteArray body;
  int status;
  bool requestSent;
  bool finished;
  bool failed;
};

void sendImageRequest(struct mg_connection *connection,
                      ImageDownloadContext *context) {
  if (context->requestSent) {
    return;
  }

  QByteArray host = context->host.toUtf8();
  QByteArray path = context->path.toUtf8();
  mg_printf(connection,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: BBCord/1.0\r\n"
            "Accept: image/png,image/jpeg,image/*\r\n"
            "Connection: close\r\n\r\n",
            path.constData(), host.constData());
  context->requestSent = true;
}

void imageEventHandler(struct mg_connection *connection, int event,
                       void *eventData) {
  ImageDownloadContext *context =
      static_cast<ImageDownloadContext *>(connection->fn_data);
  if (context == 0) {
    return;
  }

  switch (event) {
  case MG_EV_CONNECT:
    if (connection->is_tls) {
      QByteArray host = context->host.toUtf8();
      struct mg_tls_opts opts;
      memset(&opts, 0, sizeof(opts));
      opts.name = mg_str(host.constData());
      opts.skip_verification = true;
      mg_tls_init(connection, &opts);
    } else {
      sendImageRequest(connection, context);
    }
    break;

  case MG_EV_TLS_HS:
    sendImageRequest(connection, context);
    break;

  case MG_EV_HTTP_MSG: {
    struct mg_http_message *message =
        static_cast<struct mg_http_message *>(eventData);
    context->status = mg_http_status(message);
    if (context->status == 200 && message->body.len > 0) {
      context->body =
          QByteArray(message->body.buf, static_cast<int>(message->body.len));
    }
    context->finished = true;
    connection->is_closing = 1;
    break;
  }

  case MG_EV_ERROR:
  case MG_EV_CLOSE:
    if (!context->finished) {
      context->failed = true;
      context->finished = true;
    }
    break;

  default:
    break;
  }
}
} // namespace

AttachmentImageCacheWorker::AttachmentImageCacheWorker(QObject *parent)
    : QObject(parent) {}

AttachmentImageCacheWorker::~AttachmentImageCacheWorker() { cancelAll(); }

void AttachmentImageCacheWorker::requestImage(const QString &url,
                                              const QString &path,
                                              qint64 maxBytes) {
  QString safeUrl = url.trimmed();
  QString safePath = path.trimmed();
  if (safeUrl.isEmpty() || safePath.isEmpty()) {
    emit imageFailed(safeUrl);
    return;
  }

  QFileInfo cachedFile(safePath);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    emit imageCached(safeUrl, safePath);
    return;
  }

  QDir dir = cachedFile.dir();
  if (!dir.exists() && !dir.mkpath(".")) {
    emit imageFailed(safeUrl);
    return;
  }

  m_activeUrl = safeUrl;
  m_activePath = safePath;
  m_cancelledUrls.remove(safeUrl);

  bool ok = downloadImage(previewUrl(safeUrl), safePath, maxBytes);

  m_activeUrl.clear();
  m_activePath.clear();

  if (ok) {
    emit imageCached(safeUrl, safePath);
  } else {
    QFile::remove(safePath);
    emit imageFailed(safeUrl);
  }
}

void AttachmentImageCacheWorker::cancelImage(const QString &url) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty()) {
    return;
  }

  m_cancelledUrls.insert(safeUrl);
  if (safeUrl == m_activeUrl && !m_activePath.isEmpty()) {
    QFile::remove(m_activePath);
  }
}

void AttachmentImageCacheWorker::cancelAll() {
  if (!m_activeUrl.isEmpty()) {
    m_cancelledUrls.insert(m_activeUrl);
  }
  if (!m_activePath.isEmpty()) {
    QFile::remove(m_activePath);
  }
}

void AttachmentImageCacheWorker::onReadyRead() {}

void AttachmentImageCacheWorker::onFinished() {}

QString AttachmentImageCacheWorker::previewUrl(const QString &url) const {
  QUrl parsed(url);
  if (!parsed.isValid()) {
    return url;
  }

  parsed.addQueryItem("width", QString::number(kPreviewWidth));
  parsed.addQueryItem("quality", QString::number(kPreviewQuality));
  return parsed.toString();
}

bool AttachmentImageCacheWorker::downloadImage(const QString &url,
                                               const QString &path,
                                               qint64 maxBytes) {
  QUrl parsed(url);
  QString host = parsed.host().trimmed();
  QString requestPath = parsed.encodedPath();
  QString query = parsed.encodedQuery();
  if (host.isEmpty() || requestPath.isEmpty()) {
    return false;
  }

  if (!query.isEmpty()) {
    requestPath += "?" + query;
  }

  ImageDownloadContext context;
  context.host = host;
  context.path = requestPath;
  context.status = 0;
  context.requestSent = false;
  context.finished = false;
  context.failed = false;

  QString scheme = parsed.scheme().toLower();
  QByteArray connectUrl = QString("%1://%2")
                              .arg(scheme == "http" ? "http" : "https")
                              .arg(host)
                              .toUtf8();

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  mg_log_set(MG_LL_NONE);
  struct mg_connection *connection = mg_http_connect(
      &mgr, connectUrl.constData(), imageEventHandler, &context);
  if (connection == NULL) {
    mg_mgr_free(&mgr);
    return false;
  }

  int ticks = 0;
  while (!context.finished && ticks < kRequestTimeoutTicks &&
         !m_cancelledUrls.contains(m_activeUrl)) {
    mg_mgr_poll(&mgr, kPollIntervalMs);
    ++ticks;
  }

  mg_mgr_free(&mgr);

  if (m_cancelledUrls.contains(m_activeUrl) || context.failed ||
      context.status != 200 || context.body.isEmpty()) {
    return false;
  }

  if (maxBytes > 0 && context.body.size() > maxBytes) {
    return false;
  }

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  qint64 written = file.write(context.body);
  file.close();
  if (written != context.body.size()) {
    QFile::remove(path);
    return false;
  }

  return true;
}
