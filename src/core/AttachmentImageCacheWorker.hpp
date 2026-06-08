#ifndef AttachmentImageCacheWorker_HPP_
#define AttachmentImageCacheWorker_HPP_

#include <QObject>
#include <QSet>
#include <QString>

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
  QString previewUrl(const QString &url) const;
  bool downloadImage(const QString &url, const QString &path, qint64 maxBytes);

  QString m_activeUrl;
  QString m_activePath;
  QSet<QString> m_cancelledUrls;
};

#endif /* AttachmentImageCacheWorker_HPP_ */
