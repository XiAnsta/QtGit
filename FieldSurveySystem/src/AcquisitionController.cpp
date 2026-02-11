#include "AcquisitionController.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>


AcquisitionController::AcquisitionController(QObject *parent)
    : QObject(parent), m_projectManager(new ProjectManager(this)),
      m_deviceClient(new DeviceClient(this)), m_cloudSync(new CloudSync(this)),
      m_currentLineId(-1), m_currentPointId(-1) {
  // Connect Device Signals
  connect(m_deviceClient, &DeviceClient::dataReceived, this,
          [this](const ParsedPacket &packet) {
            if (packet.isValid) {
              m_lastRawData = packet.originalBytes;

              QJsonObject mon;
              mon["voltage"] = packet.monitor.batteryVoltage;
              mon["current"] = packet.monitor.current;
              mon["temp"] = packet.monitor.temperature;
              m_lastMonitorJson =
                  QJsonDocument(mon).toJson(QJsonDocument::Compact);

              emit newDataReady(packet);
            }
          });

  connect(m_deviceClient, &DeviceClient::errorOccurred, this,
          &AcquisitionController::message);
}

AcquisitionController::~AcquisitionController() {}

void AcquisitionController::setCurrentPoint(int lineId, int pointNumber) {
  m_currentLineId = lineId;
  m_currentPointNumber = pointNumber;

  // Create point in DB if not exists? Or wait until save?
  // Requirement says "Before start acquisition, manually input point number".
  // We should probably ensure the point exists or calculate its ID.
  // Let's create/retrieve it now so we have an ID for UI to show status.
  if (m_projectManager->isProjectOpen()) {
    m_currentPointId = m_projectManager->createPoint(lineId, pointNumber);
  }
}

void AcquisitionController::startAcquisition() {
  if (!m_deviceClient->isConnected()) {
    emit message("Device not connected!");
    return;
  }
  m_deviceClient->startAcquisition();
}

void AcquisitionController::stopAcquisition() {
  m_deviceClient->stopAcquisition();
}

void AcquisitionController::setAcquisitionParams(const QString &jsonParams) {
  m_currentParams = jsonParams;
  // Potentially send to device?
  // m_deviceClient->sendCommand("CONFIG:" + jsonParams);
}

void AcquisitionController::saveCurrentData() {
  if (m_currentPointId == -1) {
    emit message("No valid point selected!");
    return;
  }
  if (m_lastRawData.isEmpty()) {
    emit message("No data to save!");
    return;
  }

  bool success = m_projectManager->savePointData(
      m_currentPointId, m_lastRawData, m_currentParams, m_lastMonitorJson);
  if (success) {
    emit pointSaved(m_currentPointId);
    emit message("Point " + QString::number(m_currentPointNumber) + " saved.");

    // Trigger auto-sync
    m_cloudSync->addPendingPoint(m_currentPointId);
    m_cloudSync->syncNow();

    // Clean up last data to prevent double save?
    // m_lastRawData.clear(); // Optional, depends on workflow (if allowed to
    // save same data twice)
  }
}
