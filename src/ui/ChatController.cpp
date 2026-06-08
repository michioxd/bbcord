#include "ChatController.hpp"

#include "../core/AppStore.hpp"
#include "../core/AttachmentImageCacheWorker.hpp"
#include "../core/Client.hpp"
#include "../core/models/Models.hpp"

#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QThread>
#include <QUrl>

namespace {
const qint64 kMaxPendingAttachmentBytes = 8 * 1024 * 1024;
const qint64 kMaxCachedAttachmentImageBytes = 2 * 1024 * 1024;
} // namespace

ChatController::ChatController(DiscordClient *client, AppStore *store,
                               QObject *parent)
    : QObject(parent), m_client(client), m_store(store), m_imageThread(0),
      m_imageWorker(0), m_pendingAttachmentIsImage(false) {
  if (m_client) {
    connect(this, SIGNAL(initialMessagesRequested(QString, QString)), m_client,
            SLOT(loadInitialChatMessages(QString, QString)));
    connect(this, SIGNAL(olderMessagesRequested(QString, QString)), m_client,
            SLOT(loadOlderChatMessages(QString, QString)));
    connect(this,
            SIGNAL(sendMessageRequested(QString, QString, QString, QString,
                                        QString)),
            m_client,
            SLOT(sendChatMessage(QString, QString, QString, QString, QString)));
    connect(this, SIGNAL(editMessageRequested(QString, QString, QString)),
            m_client, SLOT(editChatMessage(QString, QString, QString)));
    connect(this, SIGNAL(deleteMessageRequested(QString, QString)), m_client,
            SLOT(deleteChatMessage(QString, QString)));
  }
}

ChatController::~ChatController() {
  if (m_imageWorker != 0) {
    QMetaObject::invokeMethod(m_imageWorker, "cancelAll",
                              Qt::BlockingQueuedConnection);
  }

  if (m_imageThread != 0) {
    m_imageThread->quit();
    m_imageThread->wait(2000);
  }
}

QString ChatController::currentChannelId() const { return m_currentChannelId; }

QString ChatController::currentGuildId() const { return m_currentGuildId; }

QString ChatController::currentChannelName() const {
  return m_currentChannelName;
}

bool ChatController::hasPendingAttachment() const {
  return !m_pendingAttachmentPath.isEmpty();
}

QString ChatController::pendingAttachmentName() const {
  return m_pendingAttachmentName;
}

QString ChatController::pendingAttachmentPreview() const {
  return m_pendingAttachmentPreview;
}

bool ChatController::pendingAttachmentIsImage() const {
  return m_pendingAttachmentIsImage;
}

QString ChatController::pendingAttachmentError() const {
  return m_pendingAttachmentError;
}

void ChatController::openChannel(const QString &channelId,
                                 const QString &guildId,
                                 const QString &channelName) {
  QString safeChannelId = channelId.trimmed();
  if (safeChannelId.isEmpty()) {
    return;
  }

  bool changed = m_currentChannelId != safeChannelId ||
                 m_currentGuildId != guildId.trimmed() ||
                 m_currentChannelName != channelName;

  m_currentChannelId = safeChannelId;
  m_currentGuildId = guildId.trimmed();
  m_currentChannelName = channelName;

  if (m_client) {
    m_client->selectChannel(safeChannelId);
  } else if (m_store) {
    m_store->selectChannel(safeChannelId);
  }

  if (changed) {
    emit currentChannelChanged();
  }

  requestInitialMessages();
}

void ChatController::closeChannel() {
  if (m_currentChannelId.isEmpty() && m_currentGuildId.isEmpty() &&
      m_currentChannelName.isEmpty()) {
    return;
  }

  m_currentChannelId.clear();
  m_currentGuildId.clear();
  m_currentChannelName.clear();
  emit currentChannelChanged();
}

QVariantList ChatController::currentMessages() const {
  return m_store ? m_store->messagesForChannel(safeCurrentChannelId())
                 : QVariantList();
}

QVariantList
ChatController::messagesForChannel(const QString &channelId) const {
  return m_store ? m_store->messagesForChannel(channelId) : QVariantList();
}

bool ChatController::isInitialLoaded() const {
  return m_store && m_store->isChatInitialLoaded(safeCurrentChannelId());
}

bool ChatController::isLoadingInitial() const {
  return m_store && m_store->isChatLoadingInitial(safeCurrentChannelId());
}

bool ChatController::isLoadingBefore() const {
  return m_store && m_store->isChatLoadingBefore(safeCurrentChannelId());
}

bool ChatController::hasMoreBefore() const {
  return m_store && m_store->hasMoreChatBefore(safeCurrentChannelId());
}

QString ChatController::oldestMessageId() const {
  return m_store ? m_store->oldestChatMessageId(safeCurrentChannelId())
                 : QString();
}

QString ChatController::newestMessageId() const {
  return m_store ? m_store->newestChatMessageId(safeCurrentChannelId())
                 : QString();
}

void ChatController::requestInitialMessages() {
  QString channelId = safeCurrentChannelId();
  if (channelId.isEmpty() || !m_store) {
    return;
  }

  if (m_store->isChatInitialLoaded(channelId) ||
      m_store->isChatLoadingInitial(channelId)) {
    return;
  }

  m_store->setChatLoadingInitial(channelId, true);
  emit initialMessagesRequested(channelId, m_currentGuildId);
}

void ChatController::requestOlderMessages() {
  QString channelId = safeCurrentChannelId();
  if (channelId.isEmpty() || !m_store) {
    return;
  }

  if (!m_store->isChatInitialLoaded(channelId) ||
      m_store->isChatLoadingBefore(channelId) ||
      !m_store->hasMoreChatBefore(channelId)) {
    return;
  }

  QString beforeMessageId = m_store->oldestChatMessageId(channelId);
  if (beforeMessageId.isEmpty()) {
    return;
  }

  m_store->setChatLoadingBefore(channelId, true);
  emit olderMessagesRequested(channelId, beforeMessageId);
}

bool ChatController::setPendingAttachment(const QString &filePath) {
  QString safePath = filePath.trimmed();
  if (safePath.startsWith("file://")) {
    safePath = QUrl(safePath).toLocalFile();
  }

  QFileInfo fileInfo(safePath);
  if (safePath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
    m_pendingAttachmentError = tr("Attachment file not found");
    emit pendingAttachmentChanged();
    return false;
  }

  if (fileInfo.size() > kMaxPendingAttachmentBytes) {
    m_pendingAttachmentError = tr("Attachment is too large");
    emit pendingAttachmentChanged();
    return false;
  }

  m_pendingAttachmentPath = fileInfo.absoluteFilePath();
  m_pendingAttachmentName = fileInfo.fileName();
  m_pendingAttachmentIsImage = isImageFile(m_pendingAttachmentPath);
  m_pendingAttachmentPreview = m_pendingAttachmentIsImage
                                   ? filePreviewSource(m_pendingAttachmentPath)
                                   : QString();
  m_pendingAttachmentError.clear();
  emit pendingAttachmentChanged();
  return true;
}

void ChatController::clearPendingAttachment() {
  if (m_pendingAttachmentPath.isEmpty() && m_pendingAttachmentName.isEmpty() &&
      m_pendingAttachmentPreview.isEmpty() && !m_pendingAttachmentIsImage) {
    return;
  }

  m_pendingAttachmentPath.clear();
  m_pendingAttachmentName.clear();
  m_pendingAttachmentPreview.clear();
  m_pendingAttachmentError.clear();
  m_pendingAttachmentIsImage = false;
  emit pendingAttachmentChanged();
}

QString ChatController::sendMessage(const QString &content,
                                    const QString &replyMessageId,
                                    const QString &replyAuthor,
                                    const QString &replyMessage) {
  QString channelId = safeCurrentChannelId();
  QString trimmedContent = content.trimmed();
  QString attachmentPath = m_pendingAttachmentPath;
  bool hasAttachment = !attachmentPath.isEmpty();
  if (channelId.isEmpty() || (!hasAttachment && trimmedContent.isEmpty()) ||
      !m_store) {
    return QString();
  }

  QString pendingId = buildPendingMessageId();
  DiscordMessage message;
  message.id = pendingId;
  message.channelId = channelId;
  message.guildId = m_currentGuildId;
  message.nonce = pendingId;
  message.content = trimmedContent;
  message.timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  message.replyMessageId = replyMessageId.trimmed();
  message.replyAuthor = replyAuthor;
  message.replyContent = replyMessage;
  message.author.id = m_store->currentUserId();
  message.author.username = m_store->currentUserName();
  message.author.globalName = m_store->currentUserName();

  if (hasAttachment) {
    DiscordAttachment attachment;
    QFileInfo fileInfo(attachmentPath);
    attachment.id = pendingId;
    attachment.filename = fileInfo.fileName();
    attachment.url = filePreviewSource(attachmentPath);
    attachment.proxyUrl = attachment.url;
    attachment.contentType = fileContentType(attachmentPath);
    attachment.size = static_cast<int>(fileInfo.size());
    if (attachment.isImage()) {
      attachment.width = 24;
      attachment.height = 18;
    }
    message.attachments.append(attachment);
  }

  m_store->addPendingChatMessage(message);
  emit sendMessageRequested(channelId, trimmedContent, pendingId,
                            message.replyMessageId, attachmentPath);
  clearPendingAttachment();
  return pendingId;
}

void ChatController::editMessage(const QString &messageId,
                                 const QString &content) {
  QString channelId = safeCurrentChannelId();
  QString safeMessageId = messageId.trimmed();
  QString trimmedContent = content.trimmed();
  if (channelId.isEmpty() || safeMessageId.isEmpty() ||
      trimmedContent.isEmpty() || !m_store) {
    return;
  }

  DiscordMessage message;
  message.id = safeMessageId;
  message.channelId = channelId;
  message.guildId = m_currentGuildId;
  message.content = trimmedContent;
  message.editedTimestamp =
      QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  m_store->updateChatMessage(message);
  emit editMessageRequested(channelId, safeMessageId, trimmedContent);
}

void ChatController::deleteMessage(const QString &messageId) {
  QString channelId = safeCurrentChannelId();
  QString safeMessageId = messageId.trimmed();
  if (channelId.isEmpty() || safeMessageId.isEmpty() || !m_store) {
    return;
  }

  m_store->deleteChatMessage(channelId, safeMessageId);
  emit deleteMessageRequested(channelId, safeMessageId);
}

void ChatController::markPendingFailed(const QString &messageId) {
  QString channelId = safeCurrentChannelId();
  QString safeMessageId = messageId.trimmed();
  if (channelId.isEmpty() || safeMessageId.isEmpty() || !m_store) {
    return;
  }

  m_store->markPendingChatMessageFailed(channelId, safeMessageId);
}

void ChatController::openAttachment(const QString &url) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty()) {
    return;
  }

  bb::system::InvokeManager manager;
  bb::system::InvokeRequest request;
  request.setTarget("sys.browser");
  request.setAction("bb.action.OPEN");
  request.setUri(QUrl(safeUrl));
  manager.invoke(request);
}

QString ChatController::avatarSource(const QString &userId,
                                     const QString &avatarHash) {
  QString safeUserId = userId.trimmed();
  QString safeAvatarHash = avatarHash.trimmed();
  if (safeUserId.isEmpty() || safeAvatarHash.isEmpty() || m_client == 0) {
    return QString();
  }

  QString source = m_client->avatarSourceForUser(safeUserId);
  if (source.isEmpty()) {
    m_client->loadUserAvatar(safeUserId, safeAvatarHash);
  }
  return source;
}

QString ChatController::cachedImageSource(const QString &url) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty()) {
    return QString();
  }

  if (!isRemoteImageUrl(safeUrl)) {
    return safeUrl;
  }

  if (m_cachedAttachmentImages.contains(safeUrl)) {
    return m_cachedAttachmentImages.value(safeUrl);
  }

  QString path = attachmentImageCachePath(safeUrl);
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    QString source = filePreviewSource(path);
    m_cachedAttachmentImages.insert(safeUrl, source);
    return source;
  }

  return QString();
}

void ChatController::requestCachedImage(const QString &url) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty() || !isRemoteImageUrl(safeUrl) ||
      m_cachedAttachmentImages.contains(safeUrl) ||
      m_loadingAttachmentImages.contains(safeUrl)) {
    return;
  }

  QString path = attachmentImageCachePath(safeUrl);
  QFileInfo cachedFile(path);
  if (cachedFile.exists() && cachedFile.size() > 0) {
    QString source = filePreviewSource(path);
    m_cachedAttachmentImages.insert(safeUrl, source);
    emit attachmentImageCached(safeUrl, source);
    return;
  }

  ensureAttachmentImageWorker();
  if (m_imageWorker == 0) {
    return;
  }

  m_loadingAttachmentImages.insert(safeUrl);
  QMetaObject::invokeMethod(m_imageWorker, "requestImage", Qt::QueuedConnection,
                            Q_ARG(QString, safeUrl), Q_ARG(QString, path),
                            Q_ARG(qint64, kMaxCachedAttachmentImageBytes));
}

void ChatController::cancelCachedImage(const QString &url) {
  QString safeUrl = url.trimmed();
  if (safeUrl.isEmpty() || !m_loadingAttachmentImages.contains(safeUrl) ||
      m_imageWorker == 0) {
    return;
  }

  m_loadingAttachmentImages.remove(safeUrl);
  QMetaObject::invokeMethod(m_imageWorker, "cancelImage", Qt::QueuedConnection,
                            Q_ARG(QString, safeUrl));
}

void ChatController::onAttachmentImageCached(const QString &url,
                                             const QString &path) {
  m_loadingAttachmentImages.remove(url);
  QString source = filePreviewSource(path);
  m_cachedAttachmentImages.insert(url, source);
  emit attachmentImageCached(url, source);
}

void ChatController::onAttachmentImageFailed(const QString &url) {
  m_loadingAttachmentImages.remove(url);
  emit attachmentImageFailed(url);
}

QString ChatController::safeCurrentChannelId() const {
  if (!m_currentChannelId.isEmpty()) {
    return m_currentChannelId;
  }

  return m_store ? m_store->selectedChannelId() : QString();
}

QString ChatController::buildPendingMessageId() const {
  return QString("pending-%1").arg(QDateTime::currentMSecsSinceEpoch());
}

QVariantMap ChatController::findCachedMessage(const QString &messageId) const {
  QVariantList messages = currentMessages();
  for (int i = 0; i < messages.size(); ++i) {
    QVariantMap message = messages.at(i).toMap();
    if (message.value("id").toString() == messageId) {
      return message;
    }
  }
  return QVariantMap();
}

QString ChatController::filePreviewSource(const QString &filePath) const {
  return QUrl::fromLocalFile(filePath).toString();
}

QString ChatController::fileContentType(const QString &filePath) const {
  QString suffix = QFileInfo(filePath).suffix().toLower();
  if (suffix == "png") {
    return "image/png";
  }
  if (suffix == "jpg" || suffix == "jpeg") {
    return "image/jpeg";
  }
  if (suffix == "gif") {
    return "image/gif";
  }
  if (suffix == "webp") {
    return "image/webp";
  }
  if (suffix == "bmp") {
    return "image/bmp";
  }
  return "application/octet-stream";
}

bool ChatController::isImageFile(const QString &filePath) const {
  return fileContentType(filePath).startsWith("image/");
}

bool ChatController::isRemoteImageUrl(const QString &url) const {
  QString safeUrl = url.trimmed().toLower();
  return safeUrl.startsWith("https://") || safeUrl.startsWith("http://");
}

QString ChatController::attachmentImageCachePath(const QString &url) const {
  QByteArray hash =
      QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex();
  QString suffix = QFileInfo(QUrl(url).path()).suffix().toLower();
  if (suffix.isEmpty()) {
    suffix = "img";
  }

  QDir dir(QDir::homePath());
  return dir.absoluteFilePath(QString("data/chat-image-cache/%1.%2")
                                  .arg(QString::fromLatin1(hash))
                                  .arg(suffix));
}

void ChatController::ensureAttachmentImageWorker() {
  if (m_imageWorker != 0) {
    return;
  }

  m_imageThread = new QThread(this);
  m_imageWorker = new AttachmentImageCacheWorker();
  m_imageWorker->moveToThread(m_imageThread);

  connect(m_imageThread, SIGNAL(finished()), m_imageWorker,
          SLOT(deleteLater()));
  connect(m_imageWorker, SIGNAL(imageCached(QString, QString)), this,
          SLOT(onAttachmentImageCached(QString, QString)));
  connect(m_imageWorker, SIGNAL(imageFailed(QString)), this,
          SLOT(onAttachmentImageFailed(QString)));

  m_imageThread->start();
}
