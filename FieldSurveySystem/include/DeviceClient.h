#ifndef DEVICECLIENT_H
#define DEVICECLIENT_H

#include "DataParser.h"
#include <QObject>
#include <QTcpSocket>


class DeviceClient : public QObject {
  Q_OBJECT
public:
  explicit DeviceClient(QObject *parent = nullptr);
  ~DeviceClient();

  void connectToDevice(const QString &ip, int port);
  void disconnectDevice();
  bool isConnected() const;

  void startAcquisition();
  void stopAcquisition();
  void sendCommand(const QString &cmd);

signals:
  void connected();
  void disconnected();
  void errorOccurred(const QString &msg);
  void dataReceived(const ParsedPacket &packet);
  void monitorDataReceived(const MonitorData &data);

private slots:
  void onReadyRead();
  void onSocketError(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState socketState);

private:
  QTcpSocket *m_socket;
  DataParser *m_parser;
  bool m_isAcquiring;
};

#endif // DEVICECLIENT_H
