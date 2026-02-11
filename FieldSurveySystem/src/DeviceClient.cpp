#include "DeviceClient.h"
#include <QHostAddress>

DeviceClient::DeviceClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)),
      m_parser(new DataParser(this)), m_isAcquiring(false) {
  connect(m_socket, &QTcpSocket::readyRead, this, &DeviceClient::onReadyRead);
  connect(m_socket, &QTcpSocket::errorOccurred, this,
          &DeviceClient::onSocketError);
  connect(m_socket, &QTcpSocket::stateChanged, this,
          &DeviceClient::onStateChanged);
}

DeviceClient::~DeviceClient() {
  if (m_socket->isOpen()) {
    m_socket->close();
  }
}

void DeviceClient::connectToDevice(const QString &ip, int port) {
  if (m_socket->state() == QAbstractSocket::ConnectedState) {
    if (m_socket->peerAddress().toString() == ip &&
        m_socket->peerPort() == port) {
      emit connected(); // Already connected
      return;
    }
    m_socket->disconnectFromHost();
  }
  m_socket->connectToHost(ip, port);
}

void DeviceClient::disconnectDevice() { m_socket->disconnectFromHost(); }

bool DeviceClient::isConnected() const {
  return m_socket->state() == QAbstractSocket::ConnectedState;
}

void DeviceClient::startAcquisition() {
  if (!isConnected())
    return;
  m_isAcquiring = true;
  sendCommand("START_ACQ"); // Custom protocol command
}

void DeviceClient::stopAcquisition() {
  if (!isConnected())
    return;
  m_isAcquiring = false;
  sendCommand("STOP_ACQ");
}

void DeviceClient::sendCommand(const QString &cmd) {
  if (m_socket->isOpen()) {
    m_socket->write(cmd.toUtf8());
  }
}

void DeviceClient::onReadyRead() {
  QByteArray data = m_socket->readAll();
  QVector<ParsedPacket> packets = m_parser->processData(data);

  for (const auto &packet : packets) {
    emit monitorDataReceived(packet.monitor);
    if (m_isAcquiring && packet.isValid) {
      emit dataReceived(packet);
    }
  }
}

void DeviceClient::onSocketError(QAbstractSocket::SocketError socketError) {
  emit errorOccurred(m_socket->errorString());
}

void DeviceClient::onStateChanged(QAbstractSocket::SocketState socketState) {
  if (socketState == QAbstractSocket::ConnectedState) {
    emit connected();
  } else if (socketState == QAbstractSocket::UnconnectedState) {
    emit disconnected();
  }
}
