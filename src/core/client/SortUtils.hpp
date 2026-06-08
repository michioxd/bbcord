#ifndef SORTUTILS_HPP_
#define SORTUTILS_HPP_

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class SortUtils : public QObject {

  Q_OBJECT
public:
  explicit SortUtils(QObject *parent = 0);

  virtual ~SortUtils();

  bool applyGuildOrder(QVariantList &guilds,
                       const QStringList &orderedGuildIds);

  bool applyGuildOrderFromGatewayPayload(QVariantList &guilds,
                                         QStringList &orderedGuildIds,
                                         const QVariantMap &payload);

  QVariantList
  sortedAccessibleGuildChannels(const QVariantList &channels) const;
};

#endif /* SORTUTILS_HPP_ */
