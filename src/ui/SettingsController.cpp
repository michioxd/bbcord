#include "SettingsController.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {
const char *kSfxEnabledKey = "sfx_enabled";
const char *kCacheDirName = "cache";
} // namespace

SettingsController::SettingsController(QObject *parent)
    : QObject(parent), m_connectionName("BBCordSettings"), m_sfxEnabled(false),
      m_cacheUsed("0 B") {
  ensureDatabase();
  m_sfxEnabled = readBool(kSfxEnabledKey, false);
  refreshCacheUsed();
}

bool SettingsController::sfxEnabled() const { return m_sfxEnabled; }

QString SettingsController::cachePath() const {
  QDir dir(QDir::homePath());
  return dir.absoluteFilePath(kCacheDirName);
}

QString SettingsController::cacheUsed() const { return m_cacheUsed; }

void SettingsController::setSfxEnabled(bool enabled) {
  if (m_sfxEnabled == enabled) {
    return;
  }

  m_sfxEnabled = enabled;
  writeBool(kSfxEnabledKey, enabled);
  emit sfxEnabledChanged(m_sfxEnabled);
}

void SettingsController::clearCache() {
  clearDirectory(cachePath());
  refreshCacheUsed();
}

void SettingsController::refreshCacheUsed() {
  ensureCacheDirectory();
  QString used = formatSize(directorySize(cachePath()));
  if (m_cacheUsed == used) {
    return;
  }

  m_cacheUsed = used;
  emit cacheUsedChanged(m_cacheUsed);
}

void SettingsController::ensureDatabase() {
  QSqlDatabase db;
  if (QSqlDatabase::contains(m_connectionName)) {
    db = QSqlDatabase::database(m_connectionName);
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    QDir dataDir(QDir::homePath());
    if (!dataDir.exists("data")) {
      dataDir.mkpath("data");
    }
    db.setDatabaseName(dataDir.absoluteFilePath("data/settings.db"));
  }

  if (!db.isOpen() && !db.open()) {
    qWarning() << "[settings] failed to open database" << db.lastError().text();
    return;
  }

  QSqlQuery query(db);
  if (!query.exec("CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, "
                  "value INTEGER NOT NULL)")) {
    qWarning() << "[settings] failed to create table"
               << query.lastError().text();
  }
}

void SettingsController::ensureCacheDirectory() const {
  QDir dir(QDir::homePath());
  if (!dir.exists(kCacheDirName)) {
    dir.mkpath(kCacheDirName);
  }
}

void SettingsController::clearDirectory(const QString &path) const {
  QDir dir(path);
  if (!dir.exists()) {
    return;
  }

  QFileInfoList entries = dir.entryInfoList(
      QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
  foreach (const QFileInfo &entry, entries) {
    if (entry.isDir()) {
      clearDirectory(entry.absoluteFilePath());
      QDir().rmdir(entry.absoluteFilePath());
    } else {
      QFile::remove(entry.absoluteFilePath());
    }
  }
}

qint64 SettingsController::directorySize(const QString &path) const {
  qint64 total = 0;
  QDir dir(path);
  QFileInfoList entries = dir.entryInfoList(
      QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
  foreach (const QFileInfo &entry, entries) {
    if (entry.isDir()) {
      total += directorySize(entry.absoluteFilePath());
    } else {
      total += entry.size();
    }
  }
  return total;
}

QString SettingsController::formatSize(qint64 bytes) const {
  if (bytes < 1024) {
    return tr("%1 B").arg(bytes);
  }

  double value = bytes;
  QStringList units;
  units << "B" << "KB" << "MB" << "GB";
  int unitIndex = 0;
  while (value >= 1024.0 && unitIndex < units.size() - 1) {
    value /= 1024.0;
    ++unitIndex;
  }

  return QString("%1 %2").arg(value, 0, 'f', 1).arg(units.at(unitIndex));
}

bool SettingsController::readBool(const QString &key, bool defaultValue) const {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if (!db.isOpen()) {
    return defaultValue;
  }

  QSqlQuery query(db);
  query.prepare("SELECT value FROM settings WHERE key = ?");
  query.addBindValue(key);
  if (!query.exec()) {
    qWarning() << "[settings] failed to read" << key
               << query.lastError().text();
    return defaultValue;
  }

  if (!query.next()) {
    return defaultValue;
  }

  return query.value(0).toInt() != 0;
}

void SettingsController::writeBool(const QString &key, bool value) {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if (!db.isOpen()) {
    return;
  }

  QSqlQuery query(db);
  query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
  query.addBindValue(key);
  query.addBindValue(value ? 1 : 0);
  if (!query.exec()) {
    qWarning() << "[settings] failed to write" << key
               << query.lastError().text();
  }
}
