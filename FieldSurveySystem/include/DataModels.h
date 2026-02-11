#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QDateTime>
#include <QString>
#include <QVariantMap>


struct SurveyLine {
  int id = -1;
  int lineNumber = 0;
};

struct SurveyPoint {
  int id = -1;
  int lineId = -1;
  int pointNumber = 0; // e.g., 1, 5, 10
  QString status;      // "measured", "saved", "synced"
  QDateTime timestamp;
};

struct SurveyData {
  int id = -1;
  int pointId = -1;
  QByteArray rawData;     // The full binary blob from device
  QString parametersJson; // Acquisition parameters
  QString monitorJson;    // Voltage, Current, monitoring info
};

#endif // DATAMODELS_H
