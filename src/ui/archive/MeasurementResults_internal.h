#ifndef MEASUREMENTRESULTS_INTERNAL_H
#define MEASUREMENTRESULTS_INTERNAL_H

// ─────────────────────────────────────────────────────────────────────────
// Приватный заголовок реализации MeasurementResults.
//
// Реализация класса разложена по нескольким .cpp (MeasurementResults.cpp,
// _style, _db, _plots, _meteo11, _export) — это по-прежнему ОДИН класс,
// просто его тела методов больше не лежат в одном файле на без малого
// четыре тысячи строк. Здесь собран общий для всех этих единиц трансляции
// набор include.
//
// Заголовок предназначен ТОЛЬКО для файлов реализации MeasurementResults;
// снаружи подключать его не нужно — публичный интерфейс класса по-прежнему
// описан в MeasurementResults.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults.h"
#include "utils/CoordHelper.h"
#include "ui_MeasurementResults.h"
#include "data/databasemanager.h"
#include "devices/ams/amsprotocol.h"
#include "core/export/MeasurementExporter.h"
#include "ui/archive/ExportDialog.h"
#include "ui/archive/ArchiveDatePopup.h"
#include "ui/archive/ArchiveExportView.h"
#include "ui/widgets/FlowLayout.h"
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QCalendarWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QGridLayout>
#include <QTabBar>
#include <QStyle>
#include <QHeaderView>
#include <QResizeEvent>
#include <QShowEvent>
#include <QScrollArea>
#include <limits>
#include <algorithm>  // Для std::sort
#include <cmath>
#include <QtMath>
#include <QTextDocument>       // Для std::pow (расчёт давления МСА в Метео-11)

#endif // MEASUREMENTRESULTS_INTERNAL_H
