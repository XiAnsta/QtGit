#ifndef DATAANALYZER_H
#define DATAANALYZER_H

#include <QObject>
#include <QString>
#include <QVector>


struct QualityResult {
  bool isGood;
  double snr; // Signal-to-Noise Ratio
  double peakVal;
  QString advice;
};

class DataAnalyzer : public QObject {
  Q_OBJECT
public:
  explicit DataAnalyzer(QObject *parent = nullptr);

  static QualityResult analyzeWaveform(const QVector<double> &data);
};

#endif // DATAANALYZER_H
