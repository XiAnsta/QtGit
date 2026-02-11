#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <QByteArray>
#include <QObject>
#include <QVector>


struct MonitorData {
  double batteryVoltage = 0.0;
  double current = 0.0;
  double temperature = 0.0;
  double signalStrength = 0.0; // Optional
};

struct ParsedPacket {
  bool isValid = false;
  MonitorData monitor;
  QVector<double> recvWaveform;
  QVector<double> sendWaveform;
  QVector<double> offWaveform;
  QByteArray originalBytes; // For storage
};

class DataParser : public QObject {
  Q_OBJECT
public:
  explicit DataParser(QObject *parent = nullptr);

  // Add data to internal buffer and try to parse
  // Returns a list of fully parsed packets
  QVector<ParsedPacket> processData(const QByteArray &newData);

  // Stateless parsing for playback
  static ParsedPacket parseBlob(const QByteArray &data);

  void setParseRule(const QString &ruleName); // Placeholder for custom rules

private:
  QByteArray m_buffer;

  // Helper to try parsing one packet from the head of m_buffer
  // Returns true if a packet was removed/parsed
  bool parseOnePacket(ParsedPacket &packet);
};

#endif // DATAPARSER_H
