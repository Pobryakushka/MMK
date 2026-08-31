#ifndef MAINWINDOW_INTERNAL_H
#define MAINWINDOW_INTERNAL_H

// ─────────────────────────────────────────────────────────────────────────
// Приватный заголовок реализации MainWindow.
//
// Реализация класса разложена по нескольким .cpp (mainwindow.cpp,
// mainwindow_sensors.cpp, _measurement, _map, _status, _overlays, _coords) —
// это по-прежнему ОДИН класс, просто его тела методов больше не лежат
// в одном файле на пять тысяч строк. Здесь собрано то, что нужно всем
// этим единицам трансляции: общий набор include и константа протокола ИВС.
//
// Заголовок предназначен ТОЛЬКО для файлов реализации MainWindow;
// снаружи подключать его не нужно — публичный интерфейс класса
// по-прежнему описан в mainwindow.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/widgets/ClickableLabel.h"
#include <QApplication>
#include "ui/widgets/RpvIndicator.h"
#include "ui/pages/SourceData.h"
#include "ui/pages/Meteo11.h"
#include "ui/pages/AlgorithmsCalc.h"
#include "ui/archive/MeasurementResults.h"
#include "ui/pages/GroundMeteoParams.h"
#include "ui/widgets/VirtualKeyboard.h"
#include "ui/theme/ScreenTheme.h"
#include "ui/pages/LandingCalculation.h"
#include "devices/ams/amshandler.h"
#include "data/databasemanager.h"
#include "utils/CoordHelper.h"
#include "data/surfacemeteosaver.h"
#include <QDateTime>
#include <QTimer>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QtPositioning/QGeoCoordinate>
#include <QPushButton>
#include <QCheckBox>
#include <QIcon>
#include <QStatusBar>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QDir>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QUrl>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEasingCurve>
#include <cmath>


// ====================================================================
// НАСТРОЙКА ПРОТОКОЛА IWS
// ====================================================================
// Измените эту константу для выбора протокола связи с IWS:
// 0 = UMB Protocol (текущие значения)
// 1 = Modbus RTU (средние значения) - рекомендуется для IWS
// ====================================================================
const int IWS_PROTOCOL = 1;  // 1 = Modbus RTU по умолчанию

#endif // MAINWINDOW_INTERNAL_H
