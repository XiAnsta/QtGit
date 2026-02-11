#ifndef ACQUISITIONCONTROLLER_H
#define ACQUISITIONCONTROLLER_H

#include "CloudSync.h"
#include "DeviceClient.h"
#include "ProjectManager.h"
#include <QObject>


class AcquisitionController : public QObject {
  Q_OBJECT
public:
  explicit AcquisitionController(QObject *parent = nullptr);
  ~AcquisitionController();

  ProjectManager *projectManager() const { return m_projectManager; }
  DeviceClient *deviceClient() const { return m_deviceClient; }
  CloudSync *cloudSync() const { return m_cloudSync; }

  // Session State
  void setCurrentPoint(int lineId, int pointNumber);
  int currentPointId() const { return m_currentPointId; }

public slots:
  void startAcquisition();
  void stopAcquisition();
  void saveCurrentData();
  void setAcquisitionParams(const QString &jsonParams);

signals:
  void pointSaved(int pointId);
  void message(const QString &msg);
  void newDataReady(const ParsedPacket &packet); // Relayed from DeviceClient

private:
  ProjectManager *m_projectManager;
  DeviceClient *m_deviceClient;
  CloudSync *m_cloudSync;

  int m_currentLineId;
  int m_currentPointNumber;
  int m_currentPointId;

  QByteArray m_lastRawData;
  QString m_lastMonitorJson;
  QString m_currentParams;
};

#endif // ACQUISITIONCONTROLLER_H
