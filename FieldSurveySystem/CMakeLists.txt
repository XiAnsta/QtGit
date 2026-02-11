cmake_minimum_required(VERSION 3.16)

project(FieldSurveySystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network Sql PrintSupport)

set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/DataAnalyzer.cpp
    src/DataExporter.cpp
    src/ProjectManager.cpp
    src/DeviceClient.cpp
    src/DataParser.cpp
    src/CloudSync.cpp
    src/AcquisitionController.cpp
    include/MainWindow.h
    include/DataAnalyzer.h
    include/DataExporter.h
    include/ProjectManager.h
    include/DeviceClient.h
    include/DataParser.h
    include/CloudSync.h
    include/AcquisitionController.h
    include/DataModels.h
    ui/MainWindow.ui
)

# QCustomPlot integration
set(QCP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QCustomPlot")
if(EXISTS "${QCP_DIR}/qcustomplot.h" AND EXISTS "${QCP_DIR}/qcustomplot.cpp")
    list(APPEND PROJECT_SOURCES 
        "${QCP_DIR}/qcustomplot.cpp"
        "${QCP_DIR}/qcustomplot.h"
    )
    add_definitions(-DUseQCustomPlot)
else()
    message(WARNING "QCustomPlot not found at ${QCP_DIR}. Plotting features will be disabled or require manual installation.")
endif()

# Application Executable
qt_add_executable(FieldSurveySystem
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(FieldSurveySystem PRIVATE include "${QCP_DIR}")

target_link_libraries(FieldSurveySystem PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql Qt6::PrintSupport)

# Copy QCustomPlot setup instructions if missing
if(NOT EXISTS "${QCP_DIR}/qcustomplot.h")
    file(WRITE "${QCP_DIR}/README.txt" "Please download QCustomPlot (qcustomplot.h and qcustomplot.cpp) and place them in this directory.\nURL: https://www.qcustomplot.com/index.php/download")
endif()
