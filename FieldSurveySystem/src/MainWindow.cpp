#include "../include/MainWindow.h"
#include "ui_MainWindow.h"
#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      m_controller(new AcquisitionController(this)) {
  ui->setupUi(this);
  setupUi(); // Build custom UI

  // Connect Controller Signals
  connect(
      m_controller, &AcquisitionController::message, this,
      [this](const QString &msg) { ui->statusbar->showMessage(msg, 5000); });

  connect(m_controller->projectManager(), &ProjectManager::projectOpened, this,
          &MainWindow::onProjectOpened);
  connect(m_controller, &AcquisitionController::pointSaved, this,
          &MainWindow::refreshProjectTree);

  connect(m_controller->deviceClient(), &DeviceClient::connected, this,
          [this]() {
            m_lblStatus->setText("Connected");
            m_lblStatus->setStyleSheet("color: green; font-weight: bold;");
            m_btnConnect->setText("Disconnect");
          });

  connect(m_controller->deviceClient(), &DeviceClient::disconnected, this,
          [this]() {
            m_lblStatus->setText("Disconnected");
            m_lblStatus->setStyleSheet("color: red;");
            m_btnConnect->setText("Connect");
          });

  connect(m_controller->deviceClient(), &DeviceClient::monitorDataReceived,
          this, &MainWindow::updateMonitor);

  connect(m_controller, &AcquisitionController::newDataReady, this,
          &MainWindow::updatePlot);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupUi() {
  // Ensure we have a central widget layout
  QWidget *central = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(central);
  setCentralWidget(central);

  QSplitter *splitter = new QSplitter(Qt::Horizontal, central);
  mainLayout->addWidget(splitter);

  // LEFT PANEL (Tree)
  QWidget *leftContainer = new QWidget();
  QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);

  QPushButton *btnNew = new QPushButton("New Project");
  QPushButton *btnOpen = new QPushButton("Open Project");
  QHBoxLayout *projBtnLayout = new QHBoxLayout();
  projBtnLayout->addWidget(btnNew);
  projBtnLayout->addWidget(btnOpen);

  m_projectTree = new QTreeWidget();
  m_projectTree->setHeaderLabels({"Line / Point", "Status", "Timestamp"});

  leftLayout->addLayout(projBtnLayout);
  leftLayout->addWidget(m_projectTree);

  splitter->addWidget(leftContainer);

  // CENTER PANEL (PLOT)
  QWidget *centerContainer = new QWidget();
  QVBoxLayout *centerLayout = new QVBoxLayout(centerContainer);
  centerLayout->addWidget(new QLabel("Real-time Waveforms", centerContainer));

#ifdef UseQCustomPlot
  m_plot = new QCustomPlot();
  m_graphRecv = m_plot->addGraph();
  m_graphRecv->setPen(QPen(Qt::blue));
  m_graphRecv->setName("Recv");

  m_graphSend = m_plot->addGraph();
  m_graphSend->setPen(QPen(Qt::red));
  m_graphSend->setName("Send");

  m_graphOff = m_plot->addGraph();
  m_graphOff->setPen(QPen(Qt::green));
  m_graphOff->setName("Off");

  m_plot->legend->setVisible(true);
  m_plot->xAxis->setLabel("Time");
  m_plot->yAxis->setLabel("Amplitude");
  m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

  centerLayout->addWidget(m_plot);
#else
  m_plotPlaceholder = new QWidget();
  m_plotPlaceholder->setStyleSheet(
      "background-color: black; border: 2px solid gray;");
  QVBoxLayout *phLayout = new QVBoxLayout(m_plotPlaceholder);
  m_plotLabel =
      new QLabel("QCustomPlot not found.\nCheck 3rdparty/QCustomPlot.");
  m_plotLabel->setStyleSheet("color: white; font-size: 14px;");
  m_plotLabel->setAlignment(Qt::AlignCenter);
  phLayout->addWidget(m_plotLabel);
  centerLayout->addWidget(m_plotPlaceholder);
#endif

  splitter->addWidget(centerContainer);

  // RIGHT PANEL (CONTROLS)
  QWidget *rightContainer = new QWidget();
  rightContainer->setMaximumWidth(320);
  QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);

  // Connection
  QGroupBox *grpConn = new QGroupBox("Device Connection");
  QFormLayout *frmConn = new QFormLayout(grpConn);
  m_editIp = new QLineEdit("127.0.0.1"); // Default local for testing
  m_spinPort = new QSpinBox();
  m_spinPort->setRange(1, 65535);
  m_spinPort->setValue(1234);
  m_btnConnect = new QPushButton("Connect");

  frmConn->addRow("IP:", m_editIp);
  frmConn->addRow("Port:", m_spinPort);
  frmConn->addRow(m_btnConnect);
  rightLayout->addWidget(grpConn);

  // Status
  m_lblStatus = new QLabel("Not Connected");
  m_lblStatus->setAlignment(Qt::AlignCenter);
  m_lblStatus->setStyleSheet(
      "font-size: 14px; font-weight: bold; color: gray;");
  rightLayout->addWidget(m_lblStatus);

  // Params
  QGroupBox *grpParams = new QGroupBox("Acquisition Info");
  QFormLayout *frmParams = new QFormLayout(grpParams);
  m_spinLine = new QSpinBox();
  m_spinLine->setRange(1, 9999);
  m_spinPoint = new QSpinBox();
  m_spinPoint->setRange(0, 9999);

  frmParams->addRow("Line No:", m_spinLine);
  frmParams->addRow("Point No:", m_spinPoint);
  rightLayout->addWidget(grpParams);

  // Actions
  m_btnStart = new QPushButton("START Acquisition");
  m_btnStart->setMinimumHeight(50);
  m_btnStart->setStyleSheet("background-color: #4CAF50; color: white; "
                            "font-weight: bold; font-size: 14px;");
  m_btnStop = new QPushButton("STOP");
  m_btnSave = new QPushButton("SAVE Point Data");
  m_btnSave->setMinimumHeight(40);
  m_btnSave->setStyleSheet(
      "background-color: #2196F3; color: white; font-weight: bold;");

  m_btnExport = new QPushButton("Export Data (CSV)"); // New

  rightLayout->addWidget(m_btnStart);
  rightLayout->addWidget(m_btnStop);
  rightLayout->addSpacing(10);
  rightLayout->addWidget(m_btnSave);
  rightLayout->addWidget(m_btnExport);

  // Monitor
  QGroupBox *grpMon = new QGroupBox("Device Monitor");
  QFormLayout *frmMon = new QFormLayout(grpMon);
  m_lblVolt = new QLabel("-- V");
  m_lblCurr = new QLabel("-- A");
  m_lblTemp = new QLabel("-- C");
  frmMon->addRow("Voltage:", m_lblVolt);
  frmMon->addRow("Current:", m_lblCurr);
  frmMon->addRow("Temp:", m_lblTemp);
  rightLayout->addWidget(grpMon);

  rightLayout->addStretch();
  splitter->addWidget(rightContainer);

  // Set Streights
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 4);
  splitter->setStretchFactor(2, 0);

  // Connect UI Signals
  connect(btnNew, &QPushButton::clicked, this, &MainWindow::onNewProject);
  connect(btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenProject);
  connect(m_btnConnect, &QPushButton::clicked, this,
          &MainWindow::onConnectDevice);
  connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartAcq);
  connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopAcq);
  connect(m_btnSave, &QPushButton::clicked, this, &MainWindow::onSaveData);
  connect(m_btnExport, &QPushButton::clicked, this, &MainWindow::onExportData);

  // PLAYBACK LOGIC
  connect(m_projectTree, &QTreeWidget::itemClicked, this,
          [this](QTreeWidgetItem *item, int column) {
            Q_UNUSED(column);
            QVariant idVar = item->data(0, Qt::UserRole);
            if (idVar.isValid()) {
              int pointId = idVar.toInt();
              SurveyData data =
                  m_controller->projectManager()->getDataForPoint(pointId);
              if (!data.rawData.isEmpty()) {
                ParsedPacket pkt = DataParser::parseBlob(data.rawData);
                if (pkt.isValid) {
                  updatePlot(pkt);
                  updateMonitor(pkt.monitor);
                  // Playback uses the same plot, analysis will also trigger
                } else {
                  ui->statusbar->showMessage("Failed to parse data for Point " +
                                             QString::number(pointId));
                }
              }
            }
          });
}

void MainWindow::onNewProject() {
  QString path = QFileDialog::getSaveFileName(this, "New Project", "",
                                              "SQLite DB (*.sqlite)");
  if (!path.isEmpty()) {
    m_controller->projectManager()->createProject(path);
  }
}

void MainWindow::onOpenProject() {
  QString path = QFileDialog::getOpenFileName(this, "Open Project", "",
                                              "SQLite DB (*.sqlite)");
  if (!path.isEmpty()) {
    m_controller->projectManager()->openProject(path);
  }
}

void MainWindow::onConnectDevice() {
  if (m_controller->deviceClient()->isConnected()) {
    m_controller->deviceClient()->disconnectDevice();
  } else {
    m_controller->deviceClient()->connectToDevice(m_editIp->text(),
                                                  m_spinPort->value());
  }
}

void MainWindow::onStartAcq() {
  m_controller->setCurrentPoint(m_spinLine->value(), m_spinPoint->value());

  QJsonObject params;
  params["line"] = m_spinLine->value();
  params["point"] = m_spinPoint->value();
  m_controller->setAcquisitionParams(QJsonDocument(params).toJson());

  m_controller->startAcquisition();
}

void MainWindow::onStopAcq() { m_controller->stopAcquisition(); }

void MainWindow::onSaveData() {
  m_controller->saveCurrentData();
  // Auto increment point
  m_spinPoint->setValue(m_spinPoint->value() + 1);
}

void MainWindow::onExportData() {
  if (!m_controller->projectManager()->isProjectOpen()) {
    QMessageBox::warning(this, "Export", "No project open.");
    return;
  }

  QString path =
      QFileDialog::getSaveFileName(this, "Export CSV", "", "CSV Files (*.csv)");
  if (path.isEmpty())
    return;

  // Fetch all needed data
  // Optimized: In real app we might query incrementally, but here we load all.
  QVector<SurveyLine> lines = m_controller->projectManager()->getAllLines();
  QVector<SurveyPoint> allPoints;
  QVector<SurveyData> allData;

  for (const auto &line : lines) {
    auto points = m_controller->projectManager()->getPointsForLine(line.id);
    allPoints.append(points);
    for (const auto &pt : points) {
      allData.append(m_controller->projectManager()->getDataForPoint(pt.id));
    }
  }

  if (DataExporter::exportToCSV(path, allPoints, allData)) {
    QMessageBox::information(this, "Export", "Export successful!");
  } else {
    QMessageBox::critical(this, "Export", "Failed to write file.");
  }
}

void MainWindow::onProjectOpened(const QString &path) {
  setWindowTitle("Field Survey System - " + path);
  refreshProjectTree();
  m_controller->setCurrentPoint(m_spinLine->value(),
                                m_spinPoint->value()); // Re-sync logic
}

void MainWindow::refreshProjectTree() {
  m_projectTree->clear();
  auto lines = m_controller->projectManager()->getAllLines();
  for (const auto &line : lines) {
    QTreeWidgetItem *lineItem = new QTreeWidgetItem(m_projectTree);
    lineItem->setText(0, "Line " + QString::number(line.lineNumber));

    auto points = m_controller->projectManager()->getPointsForLine(line.id);
    for (const auto &pt : points) {
      QTreeWidgetItem *ptItem = new QTreeWidgetItem(lineItem);
      ptItem->setText(0, "Point " + QString::number(pt.pointNumber));
      ptItem->setText(1, pt.status);
      ptItem->setText(2, pt.timestamp.toString("HH:mm:ss"));
      ptItem->setData(0, Qt::UserRole, pt.id); // Store ID for playback

      // Highlight current sync status?
      if (pt.status == "saved") {
        ptItem->setForeground(1, Qt::darkGreen);
      }
    }
    lineItem->setExpanded(true);
  }
}

void MainWindow::updateMonitor(const MonitorData &d) {
  m_lblVolt->setText(QString::number(d.batteryVoltage, 'f', 1) + " V");
  m_lblCurr->setText(QString::number(d.current, 'f', 2) + " A");
  m_lblTemp->setText(QString::number(d.temperature, 'f', 1) + " C");
}

void MainWindow::updatePlot(const ParsedPacket &packet) {
#ifdef UseQCustomPlot
  if (!packet.isValid)
    return;

  // AI Analysis
  QualityResult q = DataAnalyzer::analyzeWaveform(packet.recvWaveform);
  m_lblStatus->setText(
      QString("SNR: %1 dB - %2").arg(QString::number(q.snr, 'f', 1), q.advice));
  if (q.isGood) {
    m_lblStatus->setStyleSheet(
        "font-size: 14px; font-weight: bold; color: green;");
  } else {
    m_lblStatus->setStyleSheet(
        "font-size: 14px; font-weight: bold; color: red;");
  }

  // Create time axis (indices)
  int N = packet.recvWaveform.size();
  QVector<double> x(N);
  for (int i = 0; i < N; ++i)
    x[i] = i;

  m_graphRecv->setData(x, packet.recvWaveform);
  m_graphSend->setData(x, packet.sendWaveform);
  m_graphOff->setData(x, packet.offWaveform);

  m_plot->rescaleAxes();
  m_plot->replot(QCustomPlot::rpQueuedReplot); // Efficient update
#else
  Q_UNUSED(packet);
#endif
}
