#ifndef ImagePreview_HPP_
#define ImagePreview_HPP_

#include <QObject>
#include <QString>

class AttachmentImageCacheWorker;
class QThread;

class ImagePreview : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString imageSource READ imageSource NOTIFY stateChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
  Q_PROPERTY(bool failed READ failed NOTIFY stateChanged)
  Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
  Q_PROPERTY(int imageWidth READ imageWidth NOTIFY stateChanged)
  Q_PROPERTY(int imageHeight READ imageHeight NOTIFY stateChanged)

public:
  explicit ImagePreview(QObject *parent = 0);
  ~ImagePreview();

  QString imageSource() const;
  bool loading() const;
  bool failed() const;
  int progress() const;
  QString statusText() const;
  int imageWidth() const;
  int imageHeight() const;

  Q_INVOKABLE void open(const QString &url);
  Q_INVOKABLE void open(const QString &url, int imageWidth, int imageHeight);
  Q_INVOKABLE void close();
  Q_INVOKABLE void saveImage();

Q_SIGNALS:
  void stateChanged();
  void saveSucceeded(const QString &path);
  void saveFailed(const QString &message);

private Q_SLOTS:
  void onImageCached(const QString &url, const QString &path);
  void onImageFailed(const QString &url);
  void onImageProgress(const QString &url, qint64 receivedBytes,
                       qint64 totalBytes);

private:
  QString fileImageSource(const QString &path) const;
  QString cachePathForUrl(const QString &url) const;
  QString sharedPhotosDirPath() const;
  QString galleryPathForCurrentImage() const;
  void ensureWorker();
  void cleanupCurrentCache();
  void setStatus(const QString &text);

  QThread *m_thread;
  AttachmentImageCacheWorker *m_worker;
  QString m_url;
  QString m_cachePath;
  QString m_imageSource;
  QString m_statusText;
  int m_imageWidth;
  int m_imageHeight;
  bool m_loading;
  bool m_failed;
  int m_progress;
};

#endif /* ImagePreview_HPP_ */
