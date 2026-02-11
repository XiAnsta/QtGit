#include "DataAnalyzer.h"
#include <QtMath>
#include <numeric>

DataAnalyzer::DataAnalyzer(QObject *parent) : QObject(parent) {}

QualityResult DataAnalyzer::analyzeWaveform(const QVector<double> &data) {
  QualityResult result;
  if (data.isEmpty()) {
    result.isGood = false;
    result.snr = 0.0;
    result.peakVal = 0.0;
    result.advice = "No Data";
    return result;
  }

  // Simple SNR estimation:
  // Assume signal is the high energy part (peak) and noise is the low energy
  // tail. This is a heuristic for TEM/Geophysics pulses.

  double maxVal = -1e9;
  double minVal = 1e9;
  double sumSq = 0.0;

  for (double v : data) {
    if (v > maxVal)
      maxVal = v;
    if (v < minVal)
      minVal = v;
    sumSq += v * v;
  }

  double peakToPeak = maxVal - minVal;
  double rms = qSqrt(sumSq / data.size());

  // Heuristic: If Peak-to-Peak is significantly larger than RMS, we have a
  // pulse. Noise floor estimation is hard without knowing where the pulse is
  // off. Let's assume the last 10% of data is noise (TEM decay).

  int noiseStart = data.size() * 0.9;
  double noiseSumSq = 0.0;
  int noiseCount = 0;

  for (int i = noiseStart; i < data.size(); ++i) {
    noiseSumSq += data[i] * data[i];
    noiseCount++;
  }

  double noiseRms = (noiseCount > 0) ? qSqrt(noiseSumSq / noiseCount) : 1.0;
  if (noiseRms < 1e-6)
    noiseRms = 1e-6; // Avoid div by zero

  result.snr = 20 * qLn(peakToPeak / noiseRms) / qLn(10); // dB
  result.peakVal = peakToPeak;

  // Criteria
  if (result.snr > 20.0) {
    result.isGood = true;
    result.advice = "Quality Good. Save recommended.";
  } else if (result.snr > 10.0) {
    result.isGood = true;
    result.advice = "Quality Fair. Check connections.";
  } else {
    result.isGood = false;
    result.advice = "High Noise! Suggest Re-measure.";
  }

  return result;
}
