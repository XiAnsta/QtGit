#include "ProjectManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>


ProjectManager::ProjectManager(QObject *parent) : QObject(parent) {}

ProjectManager::~ProjectManager() { closeProject(); }

bool ProjectManager::createProject(const QString &filePath) {
  closeProject();

  m_db = QSqlDatabase::addDatabase("QSQLITE");
  m_db.setDatabaseName(filePath);

  if (!m_db.open()) {
    emit errorOccurred("Failed to create database: " + m_db.lastError().text());
    return false;
  }

  if (!initTables()) {
    m_db.close();
    return false;
  }

  m_currentFilePath = filePath;
  emit projectOpened(filePath);
  return true;
}

bool ProjectManager::openProject(const QString &filePath) {
  closeProject();

  if (!QFile::exists(filePath)) {
    emit errorOccurred("File does not exist: " + filePath);
    return false;
  }

  m_db = QSqlDatabase::addDatabase("QSQLITE");
  m_db.setDatabaseName(filePath);

  if (!m_db.open()) {
    emit errorOccurred("Failed to open database: " + m_db.lastError().text());
    return false;
  }

  // Verify schema or tables exist?
  // For now assume valid if opens. We could check tables here.

  m_currentFilePath = filePath;
  emit projectOpened(filePath);
  return true;
}

void ProjectManager::closeProject() {
  if (m_db.isOpen()) {
    m_db.close();
  }
  // Remove database connection
  QString connectionName = m_db.connectionName();
  m_db = QSqlDatabase();
  QSqlDatabase::removeDatabase(connectionName);

  m_currentFilePath.clear();
  emit projectClosed();
}

bool ProjectManager::isProjectOpen() const { return m_db.isOpen(); }

QString ProjectManager::currentProjectFile() const { return m_currentFilePath; }

bool ProjectManager::initTables() {
  if (!m_db.isOpen())
    return false;

  QSqlQuery query;

  // Lines Table
  bool success = query.exec("CREATE TABLE IF NOT EXISTS lines ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "line_number INTEGER UNIQUE)");
  if (!success) {
    emit errorOccurred("Create Lines table failed: " +
                       query.lastError().text());
    return false;
  }

  // Points Table
  success = query.exec("CREATE TABLE IF NOT EXISTS points ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "line_id INTEGER, "
                       "point_number INTEGER, "
                       "status TEXT, "
                       "timestamp DATETIME, "
                       "FOREIGN KEY(line_id) REFERENCES lines(id))");
  if (!success) {
    emit errorOccurred("Create Points table failed: " +
                       query.lastError().text());
    return false;
  }

  // Data Table
  success = query.exec("CREATE TABLE IF NOT EXISTS data ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "point_id INTEGER, "
                       "raw_data BLOB, "
                       "parameters TEXT, "
                       "monitor_data TEXT, "
                       "FOREIGN KEY(point_id) REFERENCES points(id))");
  if (!success) {
    emit errorOccurred("Create Data table failed: " + query.lastError().text());
    return false;
  }

  return true;
}

int ProjectManager::createLine(int lineNumber) {
  if (!m_db.isOpen())
    return -1;

  QSqlQuery query;
  query.prepare("INSERT OR IGNORE INTO lines (line_number) VALUES (:num)");
  query.bindValue(":num", lineNumber);

  if (!query.exec()) {
    emit errorOccurred("Failed to create line: " + query.lastError().text());
    return -1;
  }

  // If inserted, get ID. If ignored (exists), get existing ID.
  if (query.numRowsAffected() == 0) {
    query.prepare("SELECT id FROM lines WHERE line_number = :num");
    query.bindValue(":num", lineNumber);
    if (query.exec() && query.next()) {
      return query.value(0).toInt();
    }
  } else {
    return query.lastInsertId().toInt();
  }
  return -1;
}

QVector<SurveyLine> ProjectManager::getAllLines() {
  QVector<SurveyLine> lines;
  if (!m_db.isOpen())
    return lines;

  QSqlQuery query("SELECT id, line_number FROM lines ORDER BY line_number");
  while (query.next()) {
    SurveyLine line;
    line.id = query.value(0).toInt();
    line.lineNumber = query.value(1).toInt();
    lines.append(line);
  }
  return lines;
}

int ProjectManager::createPoint(int lineId, int pointNumber) {
  if (!m_db.isOpen())
    return -1;

  // Check if point exists?
  // Requirement implies points are unique per line?
  QSqlQuery check;
  check.prepare(
      "SELECT id FROM points WHERE line_id = :lid AND point_number = :pnum");
  check.bindValue(":lid", lineId);
  check.bindValue(":pnum", pointNumber);
  if (check.exec() && check.next()) {
    return check.value(0).toInt(); // Return existing
  }

  QSqlQuery query;
  query.prepare("INSERT INTO points (line_id, point_number, status, timestamp) "
                "VALUES (:lid, :pnum, 'pending', :ts)");
  query.bindValue(":lid", lineId);
  query.bindValue(":pnum", pointNumber);
  query.bindValue(":ts", QDateTime::currentDateTime());

  if (query.exec()) {
    return query.lastInsertId().toInt();
  }
  emit errorOccurred("Failed to create point: " + query.lastError().text());
  return -1;
}

QVector<SurveyPoint> ProjectManager::getPointsForLine(int lineId) {
  QVector<SurveyPoint> points;
  if (!m_db.isOpen())
    return points;

  QSqlQuery query;
  query.prepare("SELECT id, line_id, point_number, status, timestamp FROM "
                "points WHERE line_id = :lid ORDER BY point_number");
  query.bindValue(":lid", lineId);

  if (query.exec()) {
    while (query.next()) {
      SurveyPoint p;
      p.id = query.value(0).toInt();
      p.lineId = query.value(1).toInt();
      p.pointNumber = query.value(2).toInt();
      p.status = query.value(3).toString();
      p.timestamp = query.value(4).toDateTime();
      points.append(p);
    }
  }
  return points;
}

bool ProjectManager::updatePointStatus(int pointId, const QString &status) {
  if (!m_db.isOpen())
    return false;
  QSqlQuery query;
  query.prepare("UPDATE points SET status = :st WHERE id = :id");
  query.bindValue(":st", status);
  query.bindValue(":id", pointId);
  return query.exec();
}

bool ProjectManager::savePointData(int pointId, const QByteArray &rawData,
                                   const QString &paramsJson,
                                   const QString &monitorJson) {
  if (!m_db.isOpen())
    return false;

  QSqlQuery query;
  query.prepare(
      "INSERT INTO data (point_id, raw_data, parameters, monitor_data) "
      "VALUES (:pid, :raw, :params, :mon)");
  query.bindValue(":pid", pointId);
  query.bindValue(":raw", rawData);
  query.bindValue(":params", paramsJson);
  query.bindValue(":mon", monitorJson);

  if (query.exec()) {
    updatePointStatus(pointId, "saved");
    return true;
  }
  emit errorOccurred("Failed to save data: " + query.lastError().text());
  return false;
}

SurveyData ProjectManager::getDataForPoint(int pointId) {
  SurveyData data;
  if (!m_db.isOpen())
    return data;

  QSqlQuery query;
  query.prepare("SELECT id, point_id, raw_data, parameters, monitor_data FROM "
                "data WHERE point_id = :pid");
  query.bindValue(":pid", pointId);

  if (query.exec() && query.next()) {
    data.id = query.value(0).toInt();
    data.pointId = query.value(1).toInt();
    data.rawData = query.value(2).toByteArray();
    data.parametersJson = query.value(3).toString();
    data.monitorJson = query.value(4).toString();
  }
  return data;
}
