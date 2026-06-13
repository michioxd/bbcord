#ifndef AboutController_HPP_
#define AboutController_HPP_

#include <QObject>

struct mg_mgr;
struct mg_connection;

class AboutController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool checking READ checking NOTIFY updateStateChanged)
  Q_PROPERTY(bool checked READ checked NOTIFY updateStateChanged)
  Q_PROPERTY(
      bool updateAvailable READ updateAvailable NOTIFY updateStateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY updateStateChanged)
  Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateStateChanged)
  Q_PROPERTY(
      QString publishedAtText READ publishedAtText NOTIFY updateStateChanged)
  Q_PROPERTY(QString sizeText READ sizeText NOTIFY updateStateChanged)
  Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY updateStateChanged)
  Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY updateStateChanged)

public:
  explicit AboutController(QObject *parent = 0);
  virtual ~AboutController();

  bool checking() const;
  bool checked() const;
  bool updateAvailable() const;
  QString statusText() const;
  QString latestVersion() const;
  QString publishedAtText() const;
  QString sizeText() const;
  QString releaseUrl() const;
  QString downloadUrl() const;

  Q_INVOKABLE void checkForUpdates(bool force);

Q_SIGNALS:
  void updateStateChanged();

protected:
  virtual void timerEvent(QTimerEvent *event);

private:
  static void eventHandler(struct mg_connection *connection, int event,
                           void *eventData);
  void handleEvent(struct mg_connection *connection, int event,
                   void *eventData);
  void sendUpdateRequest(struct mg_connection *connection);
  void finishRequest();
  void setChecking(bool checking);
  void setLatestState();
  void setErrorState(const QString &message);
  void parseResponse(const QByteArray &bytes);
  bool isRemoteVersionNewer(const QString &remoteVersion) const;
  QString currentVersion() const;
  QString formatPublishedAt(const QString &publishedAt) const;
  QString formatSize(qint64 bytes) const;

  mg_mgr *m_mgr;
  mg_connection *m_connection;
  int m_timerId;
  int m_pollTicks;
  bool m_checking;
  bool m_checked;
  bool m_requestSent;
  bool m_updateAvailable;
  QString m_statusText;
  QString m_latestVersion;
  QString m_publishedAtText;
  QString m_sizeText;
  QString m_releaseUrl;
  QString m_downloadUrl;
};

#endif /* AboutController_HPP_ */
