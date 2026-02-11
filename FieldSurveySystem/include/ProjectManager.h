#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "DataModels.h"
#include <QObject>
#include <QSqlDatabase>
#include <QVector>


class ProjectManager : public QObject {
  Q_OBJECT
public:
  explicit ProjectManager(QObject *parent = nullptr);
  ~ProjectManager();

  // File Operations
  bool createProject(const QString &filePath);
  bool openProject(const QString &filePath);
  void closeProject();
  bool isProjectOpen() const;
  QString currentProjectFile() const;

  // Data Operations
  // Lines
  int createLine(int lineNumber);
  QVector<SurveyLine> getAllLines();

  // Points
  int createPoint(int lineId, int pointNumber);
  QVector<SurveyPoint> getPointsForLine(int lineId);
  bool updatePointStatus(int pointId, const QString &status);

  // Data
  bool savePointData(int pointId, const QByteArray &rawData,
                     const QString &paramsJson, const QString &monitorJson);
  SurveyData getDataForPoint(int pointId);

signals:
  void projectOpened(const QString &path);
  void projectClosed();
  void errorOccurred(const QString &msg);

private:
  bool initTables();
  QSqlDatabase m_db;
  QString m_currentFilePath;
};

#endif // PROJECTMANAGER_H
