#include "DataExporter.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>


bool DataExporter::exportToCSV(const QString &filePath,
                               const QVector<SurveyPoint> &points,
                               const QVector<SurveyData> &dataList) {
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  // Header
  out << "PointID,Line,PointNumber,Status,Timestamp,Voltage(V),Current(A),Temp("
         "C)\n";

  // Build a map for faster lookup if needed, but assuming dataList corresponds
  // or we fetch Actually, ProjectManager knows the relation.

  // We iterate points
  for (const auto &pt : points) {
    // Find data for point
    SurveyData d;
    bool found = false;
    for (const auto &dd : dataList) {
      if (dd.pointId == pt.id) {
        d = dd;
        found = true;
        break;
      }
    }

    double volts = 0, amps = 0, temp = 0;
    if (found) {
      QJsonDocument doc = QJsonDocument::fromJson(d.monitorJson.toUtf8());
      QJsonObject obj = doc.object();
      volts = obj["voltage"].toDouble();
      amps = obj["current"].toDouble();
      temp = obj["temp"].toDouble();
    }

    out << pt.id << "," << pt.lineId
        << "," // Should look up Line Number ideally
        << pt.pointNumber << "," << pt.status << ","
        << pt.timestamp.toString(Qt::ISODate) << "," << volts << "," << amps
        << "," << temp << "\n";
  }

  file.close();
  return true;
}
