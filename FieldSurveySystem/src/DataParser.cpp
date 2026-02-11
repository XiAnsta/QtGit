#include "DataParser.h"
#include <QDataStream>
#include <QDebug>
#include <QtMath>

DataParser::DataParser(QObject *parent) : QObject(parent) {}

QVector<ParsedPacket> DataParser::processData(const QByteArray &newData) {
  m_buffer.append(newData);
  QVector<ParsedPacket> packets;

  while (true) {
    ParsedPacket p;
    if (parseOnePacket(p)) {
      packets.append(p);
    } else {
      break; // Need more data
    }
  }
  return packets;
}

ParsedPacket DataParser::parseBlob(const QByteArray &data) {
  DataParser tempParser;
  tempParser.m_buffer = data;
  ParsedPacket p;
  if (tempParser.parseOnePacket(p)) {
    return p;
  }
  return ParsedPacket();
}

void DataParser::setParseRule(const QString &ruleName) {
  // TODO: Implement custom parsing rules
  Q_UNUSED(ruleName);
}

bool DataParser::parseOnePacket(ParsedPacket &packet) {
  // Mock Protocol:
  // Header: 0xAAAA (2 bytes)
  // TotalLength: 4 bytes (including header)
  // Monitor: 3 doubles (24 bytes)
  // Packet MUST be at least 6 bytes

  if (m_buffer.size() < 6)
    return false;

  QDataStream stream(m_buffer);
  stream.setByteOrder(QDataStream::BigEndian); // Assume Network Byte Order

  quint16 header;
  stream >> header;

  if (header != 0xAAAA) {
    int idx = m_buffer.indexOf(QByteArray::fromHex("AAAA"));
    if (idx != -1) {
      m_buffer.remove(0, idx);
      if (m_buffer.size() < 6)
        return false;
      return false; // loop again
    } else {
      if (m_buffer.size() > 1) {
        m_buffer = m_buffer.right(1);
      }
      return false;
    }
  }

  quint32 length;
  stream >> length;

  if (m_buffer.size() < (int)length) {
    return false; // Wait for more data
  }

  // We have a full packet
  QByteArray packetData = m_buffer.left(length);
  m_buffer.remove(0, length);

  packet.originalBytes = packetData;
  packet.isValid = true;

  // Parse Payload
  QDataStream pStream(packetData);
  pStream.setByteOrder(QDataStream::BigEndian);
  pStream.skipRawData(6); // Skip Header + Length

  // Monitor Data
  if (pStream.atEnd())
    return true;

  pStream >> packet.monitor.batteryVoltage;
  pStream >> packet.monitor.current;
  pStream >> packet.monitor.temperature;

  // Waveforms (Mocking logic)
  // In a real scenario, we would parse bytes here.
  // Since we are mocking the waveform generation in the "Parse" step
  // (because the user didn't provide the exact protocol for waveforms, just
  // said "custom parsing"), we will generate them based on the fact that we got
  // a valid packet.

  packet.monitor.signalStrength = -50.0;

  // Generate Mock Waveforms
  for (int i = 0; i < 800; ++i) { // 800 pts
    double t = i * 0.1;
    packet.recvWaveform.append(qSin(t) * qExp(-t * 0.01) * 100);
    packet.sendWaveform.append((i > 100 && i < 300) ? 25.0 : 0.0);
    packet.offWaveform.append((i > 300) ? qExp(-(i - 300) * 0.02) * 10 : 0);
  }

  return true;
}
