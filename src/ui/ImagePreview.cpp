#include "ImagePreview.hpp"

#include "../core/AttachmentImageCacheWorker.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QThread>
#include <QUrl>

ImagePreview::ImagePreview(QObject *parent)
    : QObject(parent), m_thread(0), m_worker(0), m_imageWidth(0),
      m_imageHeight(0), m_loading(false), m_failed(false), m_progress(0) {}

ImagePreview::~ImagePreview() {
  close();
  if (m_worker != 0) {
    QMetaObject::invokeMethod(m_worker, "cancelAll",
                              Qt::BlockingQueuedConnection);
  }
  if (m_thread != 0) {
    m_thread->quit();
    m_thread->wait(2000);
  }
}

QString ImagePreview::imageSource() const { return m_imageSource; }

bool ImagePreview::loading() const { return m_loading; }

bool ImagePreview::failed() const { return m_failed; }

int ImagePreview::progress() const { return m_progress; }

QString ImagePreview::statusText() const { return m_statusText; }

int ImagePreview::imageWidth() const { return m_imageWidth; }

int ImagePreview::imageHeight() const { return m_imageHeight; }

void ImagePreview::open(const QString &url) { open(url, 0, 0); }

void ImagePreview::open(const QString &url, int imageWidth, int imageHeight) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty()) {
    return;
  }

  close();
  m_url = safeUrl;
  m_cachePath = cachePathForUrl(safeUrl);
  m_imageSource.clear();
  m_imageWidth = imageWidth;
  m_imageHeight = imageHeight;
  m_loading = true;
  m_failed = false;
  m_progress = 0;
  setStatus(tr("Loading image..."));
  emit stateChanged();

  ensureWorker();
  if (m_worker == 0) {
    m_loading = false;
    m_failed = true;
    setStatus(tr("Could not start image loader"));
    emit stateChanged();
    return;
  }

  QMetaObject::invokeMethod(m_worker, "requestImage", Qt::QueuedConnection,
                            Q_ARG(QString, m_url), Q_ARG(QString, m_cachePath),
                            Q_ARG(qint64, 0));
}

void ImagePreview::close() {
  if (!m_url.isEmpty() && m_worker != 0 && m_loading) {
    QMetaObject::invokeMethod(m_worker, "cancelImage", Qt::QueuedConnection,
                              Q_ARG(QString, m_url));
  }

  cleanupCurrentCache();
  m_url.clear();
  m_cachePath.clear();
  m_imageSource.clear();
  m_imageWidth = 0;
  m_imageHeight = 0;
  m_statusText.clear();
  m_loading = false;
  m_failed = false;
  m_progress = 0;
  emit stateChanged();
}

void ImagePreview::saveImage() {
  if (m_cachePath.isEmpty() || !QFileInfo(m_cachePath).exists()) {
    emit saveFailed(tr("Image is not loaded yet"));
    return;
  }

  QString outputPath = galleryPathForCurrentImage();
  QFileInfo outputInfo(outputPath);
  QDir outputDir = outputInfo.dir();
  if (!outputDir.exists() && !outputDir.mkpath(".")) {
    emit saveFailed(tr("Could not open gallery folder"));
    return;
  }

  if (QFile::exists(outputPath)) {
    QFile::remove(outputPath);
  }
  if (QFile::copy(m_cachePath, outputPath)) {
    emit saveSucceeded(outputPath);
  } else {
    emit saveFailed(tr("Could not save image"));
  }
}

void ImagePreview::onImageCached(const QString &url, const QString &path) {
  if (url != m_url) {
    return;
  }

  m_cachePath = path;
  m_imageSource = fileImageSource(path);
  m_loading = false;
  m_failed = false;
  m_progress = 100;
  m_statusText.clear();
  emit stateChanged();
}

void ImagePreview::onImageFailed(const QString &url) {
  if (url != m_url) {
    return;
  }

  m_loading = false;
  m_failed = true;
  m_progress = 0;
  setStatus(tr("Could not load image"));
  cleanupCurrentCache();
  emit stateChanged();
}

void ImagePreview::onImageProgress(const QString &url, qint64 receivedBytes,
                                   qint64 totalBytes) {
  if (url != m_url || !m_loading) {
    return;
  }

  if (totalBytes > 0) {
    int percent = static_cast<int>((receivedBytes * 100) / totalBytes);
    if (percent < 0) {
      percent = 0;
    }
    if (percent > 99) {
      percent = 99;
    }
    m_progress = percent;
    setStatus(tr("Loading image... %1%").arg(m_progress));
  }
  emit stateChanged();
}

QString ImagePreview::fileImageSource(const QString &path) const {
  QString safePath = path.trimmed();
  if (safePath.startsWith("file://") || safePath.startsWith("asset://")) {
    return safePath;
  }
  return QString("file://%1").arg(safePath);
}

QString ImagePreview::cachePathForUrl(const QString &url) const {
  QByteArray hash =
      QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();
  QString suffix = QFileInfo(QUrl(url).path()).suffix().toLower();
  if (suffix.isEmpty()) {
    suffix = "img";
  }
  QDir dir(QDir::homePath());
  return dir.absoluteFilePath(QString("cache/image-preview-cache/%1.%2")
                                  .arg(QString::fromLatin1(hash))
                                  .arg(suffix));
}

QString ImagePreview::sharedPhotosDirPath() const {
  QString home = QProcessEnvironment::systemEnvironment().value("HOME");
  if (home.isEmpty()) {
    home = QDir::homePath();
  }

  QString accountRoot;
  QStringList parts = QDir::cleanPath(home).split('/', QString::SkipEmptyParts);
  if (parts.size() >= 2 && parts.at(0) == "accounts") {
    accountRoot = QString("/accounts/%1").arg(parts.at(1));
  }

  if (accountRoot.isEmpty()) {
    accountRoot = QDir::cleanPath(home);
  }

  return QDir(accountRoot).absoluteFilePath("shared/photos");
}

QString ImagePreview::galleryPathForCurrentImage() const {
  QString suffix = QFileInfo(m_cachePath).suffix().toLower();
  if (suffix.isEmpty()) {
    suffix = "jpg";
  }

  QString fileName = QFileInfo(QUrl(m_url).path()).fileName();
  if (fileName.isEmpty()) {
    fileName = QFileInfo(m_cachePath).fileName();
  }

  QDir dir(sharedPhotosDirPath());
  return dir.absoluteFilePath(fileName);
}

void ImagePreview::ensureWorker() {
  if (m_worker != 0) {
    return;
  }

  m_thread = new QThread(this);
  m_worker = new AttachmentImageCacheWorker();
  m_worker->moveToThread(m_thread);
  connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
  connect(m_worker, SIGNAL(imageCached(QString, QString)), this,
          SLOT(onImageCached(QString, QString)));
  connect(m_worker, SIGNAL(imageFailed(QString)), this,
          SLOT(onImageFailed(QString)));
  connect(m_worker, SIGNAL(imageProgress(QString, qint64, qint64)), this,
          SLOT(onImageProgress(QString, qint64, qint64)));
  m_thread->start();
}

void ImagePreview::cleanupCurrentCache() {
  if (!m_cachePath.isEmpty()) {
    QFile::remove(m_cachePath);
  }
}

void ImagePreview::setStatus(const QString &text) { m_statusText = text; }
