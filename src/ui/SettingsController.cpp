#include "SettingsController.hpp"

#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const char *kSfxEnabledKey = "sfx_enabled";
}

SettingsController::SettingsController(QObject *parent)
    : QObject(parent), m_connectionName("BBCordSettings"), m_sfxEnabled(false) {
  ensureDatabase();
  m_sfxEnabled = readBool(kSfxEnabledKey, false);
}

bool SettingsController::sfxEnabled() const { return m_sfxEnabled; }

void SettingsController::setSfxEnabled(bool enabled) {
  if (m_sfxEnabled == enabled) {
    return;
  }

  m_sfxEnabled = enabled;
  writeBool(kSfxEnabledKey, enabled);
  emit sfxEnabledChanged(m_sfxEnabled);
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
