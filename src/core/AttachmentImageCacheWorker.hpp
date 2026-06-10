#ifndef AttachmentImageCacheWorker_HPP_
#define AttachmentImageCacheWorker_HPP_

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>

struct mg_connection;
struct mg_mgr;
class QTimerEvent;
class AttachmentImageCacheWorker;

struct ImageDownloadContext {
  QString url;
  QString filePath;
  QString host;
  QString path;
  QByteArray body;
  qint64 maxBytes;
  int status;
  int ticks;
  bool requestSent;
  bool finished;
  bool failed;
  AttachmentImageCacheWorker *worker;
};

class AttachmentImageCacheWorker : public QObject {
  Q_OBJECT

public:
  explicit AttachmentImageCacheWorker(QObject *parent = 0);
  ~AttachmentImageCacheWorker();

public Q_SLOTS:
  void requestImage(const QString &url, const QString &path, qint64 maxBytes);
  void cancelImage(const QString &url);
  void cancelAll();

private Q_SLOTS:
  void onReadyRead();
  void onFinished();

Q_SIGNALS:
  void imageCached(const QString &url, const QString &path);
  void imageFailed(const QString &url);

private:
  void timerEvent(QTimerEvent *event);
  void startTimerIfNeeded();
  void stopTimerIfIdle();
  void finishDownload(struct mg_connection *connection, bool success);
  static void imageEventHandler(struct mg_connection *connection, int event,
                                void *eventData);

  struct mg_mgr *m_mgr;
  int m_timerId;
  QMap<struct mg_connection *, ImageDownloadContext *> m_activeDownloads;
  QSet<QString> m_cancelledUrls;
};

#endif /* AttachmentImageCacheWorker_HPP_ */
