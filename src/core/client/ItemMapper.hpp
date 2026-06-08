#ifndef ITEMMAPPER_HPP_
#define ITEMMAPPER_HPP_

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class ItemMapper : public QObject {

  Q_OBJECT
public:
  explicit ItemMapper(QObject *parent = 0);

  virtual ~ItemMapper();

  QVariantMap guildToItem(const QVariantMap &guild) const;

  QVariantMap dmChannelToItem(const QVariantMap &channel,
                              const QVariantMap &avatarSourcesByUserId,
                              const QVariantMap &dmPresenceByUserId) const;

  QVariantMap guildChannelToItem(const QVariantMap &channel) const;

private:
  QString dmStatusForRecipients(const QVariantList &userIds,
                                const QVariantMap &dmPresenceByUserId) const;
};

#endif /* ITEMMAPPER_HPP_ */
