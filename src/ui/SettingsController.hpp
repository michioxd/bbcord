#ifndef SettingsController_HPP_
#define SettingsController_HPP_

#include <QObject>
#include <QString>

class SettingsController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool sfxEnabled READ sfxEnabled WRITE setSfxEnabled NOTIFY
                 sfxEnabledChanged)
  Q_PROPERTY(QString cachePath READ cachePath CONSTANT)
  Q_PROPERTY(QString cacheUsed READ cacheUsed NOTIFY cacheUsedChanged)

public:
  explicit SettingsController(QObject *parent = 0);

  bool sfxEnabled() const;
  QString cachePath() const;
  QString cacheUsed() const;
  Q_INVOKABLE void setSfxEnabled(bool enabled);
  Q_INVOKABLE void clearCache();
  Q_INVOKABLE void refreshCacheUsed();

Q_SIGNALS:
  void sfxEnabledChanged(bool enabled);
  void cacheUsedChanged(const QString &cacheUsed);

private:
  void ensureDatabase();
  void ensureCacheDirectory() const;
  void clearDirectory(const QString &path) const;
  qint64 directorySize(const QString &path) const;
  QString formatSize(qint64 bytes) const;
  bool readBool(const QString &key, bool defaultValue) const;
  void writeBool(const QString &key, bool value);

  QString m_connectionName;
  bool m_sfxEnabled;
  QString m_cacheUsed;
};

#endif /* SettingsController_HPP_ */
