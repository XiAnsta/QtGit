#include "CloudSync.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>


CloudSync::CloudSync(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)),
      m_timer(new QTimer(this)), m_isSyncing(false) {
  connect(m_timer, &QTimer::timeout, this, &CloudSync::checkNetworkAndSync);
}

void CloudSync::startAutoSync(int intervalMs) {
  m_timer->start(intervalMs);
  checkNetworkAndSync(); // Try immediately
}

void CloudSync::stopAutoSync() { m_timer->stop(); }

void CloudSync::syncNow() { checkNetworkAndSync(); }

void CloudSync::addPendingPoint(int pointId) {
  if (!m_pendingPoints.contains(pointId)) {
    m_pendingPoints.append(pointId);
  }
}

void CloudSync::checkNetworkAndSync() {
  if (m_isSyncing || m_pendingPoints.isEmpty())
    return;

  // Simple connectivity check: assume valid if NAM says so or just try
  if (m_nam->networkAccessible() == QNetworkAccessManager::NotAccessible) {
    return;
  }

  emit syncStarted();
  m_isSyncing = true;
  uploadNext();
}

void CloudSync::uploadNext() {
  if (m_pendingPoints.isEmpty()) {
    m_isSyncing = false;
    emit syncFinished(true, "All pending points synced");
    return;
  }

  int pointId = m_pendingPoints.first();

  // Construct payload (Mocking data retrieval here or usually would ask
  // ProjectManager) Since CloudSync is decoupled, we'd ideally emit a signal
  // requesting data or pass it in. For simplicity, I'll assume we construct a
  // mock JSON packet request.

  QJsonObject json;
  json["pointId"] = pointId;
  json["timestamp"] = QDateTime::currentDateTime().toString();
  // In real app, we need to fetch the blob from ProjectManager.
  // This design might need ProjectManager injection or signal/slot flow.
  // I will mock the upload request.

  QNetworkRequest request(
      QUrl("https://api.example.com/sync")); // CONFIGURABLE URL
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = m_nam->post(request, QJsonDocument(json).toJson());
  reply->setProperty("pointId", pointId);
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onReplyFinished(reply); });
}

void CloudSync::onReplyFinished(QNetworkReply *reply) {
  reply->deleteLater();
  int pointId = reply->property("pointId").toInt();

  if (reply->error() == QNetworkReply::NoError) {
    m_pendingPoints.removeOne(pointId);
    emit pointSynced(pointId);
    uploadNext();
  } else {
    m_isSyncing = false;
    emit syncFinished(false, reply->errorString());
    // Retry later via timer
  }
}
