#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTreeWidget>


#include "AcquisitionController.h"
#include "DataAnalyzer.h"
#include "DataExporter.h"

// Define UseQCustomPlot if the file exists (CMake handles this usually, but we
// safeguard here) Actually, CMake defines -DUseQCustomPlot, so we trust it.
#ifdef UseQCustomPlot
#include "qcustomplot.h"
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  // Core Actions
  void onNewProject();
  void onOpenProject();
  void onConnectDevice();
  void onStartAcq();
  void onStopAcq();
  void onSaveData();
  void onExportData(); // New

  // Updates
  void updateMonitor(const MonitorData &data);
  void updatePlot(const ParsedPacket &packet);
  void onProjectOpened(const QString &path);
  void refreshProjectTree();

private:
  void setupUi();

  Ui::MainWindow *ui;
  AcquisitionController *m_controller;

  // UI Elements
  QTreeWidget *m_projectTree;

  // Plotting
#ifdef UseQCustomPlot
  QCustomPlot *m_plot;
  QCPGraph *m_graphRecv;
  QCPGraph *m_graphSend;
  QCPGraph *m_graphOff;
#else
  QWidget *m_plotPlaceholder;
  QLabel *m_plotLabel;
#endif

  // Controls
  QLineEdit *m_editIp;
  QSpinBox *m_spinPort;
  QPushButton *m_btnConnect;

  QSpinBox *m_spinLine;
  QSpinBox *m_spinPoint;

  QPushButton *m_btnStart;
  QPushButton *m_btnStop;
  QPushButton *m_btnSave;
  QPushButton *m_btnExport; // New

  QLabel *m_lblVolt;
  QLabel *m_lblCurr;
  QLabel *m_lblTemp;
  QLabel *m_lblStatus;
};

#endif // MAINWINDOW_H
