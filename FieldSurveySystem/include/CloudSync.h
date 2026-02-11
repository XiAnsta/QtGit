#ifndef CLOUDSYNC_H
#define CLOUDSYNC_H

#include "DataModels.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>


class CloudSync : public QObject {
  Q_OBJECT
public:
  explicit CloudSync(QObject *parent = nullptr);
  void startAutoSync(int intervalMs = 60000);
  void stopAutoSync();
  void syncNow();

  void addPendingPoint(int pointId); // Queue a point for sync

signals:
  void syncStarted();
  void syncFinished(bool success, const QString &msg);
  void pointSynced(int pointId);

private slots:
  void checkNetworkAndSync();
  void onReplyFinished(QNetworkReply *reply);

private:
  QNetworkAccessManager *m_nam;
  QTimer *m_timer;
  QVector<int> m_pendingPoints;
  bool m_isSyncing;

  void uploadNext();
};

#endif // CLOUDSYNC_H
