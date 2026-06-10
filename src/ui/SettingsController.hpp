#ifndef SettingsController_HPP_
#define SettingsController_HPP_

#include <QObject>

class SettingsController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool sfxEnabled READ sfxEnabled WRITE setSfxEnabled NOTIFY
                 sfxEnabledChanged)

public:
  explicit SettingsController(QObject *parent = 0);

  bool sfxEnabled() const;
  Q_INVOKABLE void setSfxEnabled(bool enabled);

Q_SIGNALS:
  void sfxEnabledChanged(bool enabled);

private:
  void ensureDatabase();
  bool readBool(const QString &key, bool defaultValue) const;
  void writeBool(const QString &key, bool value);

  QString m_connectionName;
  bool m_sfxEnabled;
};

#endif /* SettingsController_HPP_ */
