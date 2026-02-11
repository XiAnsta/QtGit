#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include "DataModels.h"
#include <QString>
#include <QVector>


class DataExporter {
public:
  static bool exportToCSV(const QString &filePath,
                          const QVector<SurveyPoint> &points,
                          const QVector<SurveyData> &dataList);
};

#endif // DATAEXPORTER_H
