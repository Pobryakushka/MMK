#include "SourceData.h"
#include "ui_SourceData.h"
#include "Meteo11.h"
#include "sensors/GroundMeteoParams.h"
#include <QShowEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QStyle>
#include <QTimer>

SourceData::SourceData(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SourceData)
    , groundMeteoParams(nullptr)
    , m_meteo11Dialog(nullptr)
{
    ui->setupUi(this);

    groundMeteoParams = new GroundMeteoParams(this);
    qDebug() << "GroundMeteoParams instance created in SourceData";

    // Создаём ПОСТОЯННЫЙ экземпляр Meteo11 — данные не теряются
    // между открытиями диалога и сохраняются до нажатия «Пуск»
    m_meteo11Dialog = new Meteo11(this);

    // Плитки (rowMeteo11 / rowGroundParams) целиком описаны в SourceData.ui.
    // У обычного QWidget-контейнера нет сигнала clicked() — ловим клик через
    // eventFilter, как это принято для некликабельных по умолчанию виджетов.
    ui->rowMeteo11->setCursor(Qt::PointingHandCursor);
    ui->rowMeteo11->installEventFilter(this);
    ui->rowGroundParams->setCursor(Qt::PointingHandCursor);
    ui->rowGroundParams->installEventFilter(this);

    // Бейдж приземного слоя обновляется мгновенно при изменении состояния,
    // даже если сам диалог параметров сейчас не в фокусе.
    connect(groundMeteoParams, &GroundMeteoParams::surfaceStateChanged,
            this, &SourceData::updateBadges);

    connect(ui->btnSourceDataBack, &QPushButton::clicked, this, [this]() {
        emit backRequested();
    });

    updateBadges();
}

SourceData::~SourceData()
{
    delete ui;
    // m_meteo11Dialog и groundMeteoParams удаляются через parent
}

void SourceData::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Бейдж бюллетеня "Метео-11" зависит от его внутреннего состояния,
    // о котором SourceData не оповещается сигналом — освежаем при каждом
    // показе страницы, этого достаточно для актуальности на практике.
    updateBadges();
}

void SourceData::setRowPressed(QWidget *row, bool pressed)
{
    row->setProperty("pressed", pressed);
    row->style()->unpolish(row);
    row->style()->polish(row);
}

bool SourceData::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *row = qobject_cast<QWidget*>(watched);
    if (row != ui->rowMeteo11 && row != ui->rowGroundParams)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton)
            setRowPressed(row, true);
        return false; // не глотаем событие — пусть доходит и до дочерних QLabel
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            const bool wasPressed = row->property("pressed").toBool();
            setRowPressed(row, false);
            if (wasPressed && row->rect().contains(me->pos())) {
                // ВАЖНО: GroundMeteoParams больше не отдельный QDialog, а
                // встроенная страница (как AlgorithmsCalculation) — именно
                // открытие отдельного top-level окна вызывало зависание на
                // этой платформе. Здесь только просим MainWindow переключить
                // stackedWidget; сама навигация — в MainWindow.
                //
                // Meteo11 пока остаётся отдельным QDialog (его файлов у меня
                // нет) — при открытии бюллетеня зависание, скорее всего,
                // повторится тем же образом, пока и его не переведут на
                // страницу по этому же паттерну.
                const bool isMeteo11 = (row == ui->rowMeteo11);
                QTimer::singleShot(0, this, [this, isMeteo11]() {
                    if (isMeteo11) {
                        m_meteo11Dialog->show();
                        m_meteo11Dialog->raise();
                        m_meteo11Dialog->activateWindow();
                    } else {
                        qDebug() << "[TEMP DEBUG] SourceData: before emit openGroundParamsRequested()";
                        emit openGroundParamsRequested();
                        qDebug() << "[TEMP DEBUG] SourceData: after emit openGroundParamsRequested()";
                    }
                });
            }
        }
        return false;
    }

    return QWidget::eventFilter(watched, event);
}

void SourceData::updateBadges()
{
    if (hasMeteo11Bulletin()) {
        ui->badgeMeteo11->setText(QString::fromUtf8("Загружен"));
        ui->badgeMeteo11->setStyleSheet(
            "font-size:8pt; font-weight:700; padding:4px 10px; border-radius:10px;"
            "color:#0F6B4F; background-color:#E8F5E9;");
    } else {
        ui->badgeMeteo11->setText(QString::fromUtf8("Не загружен"));
        ui->badgeMeteo11->setStyleSheet(
            "font-size:8pt; font-weight:700; padding:4px 10px; border-radius:10px;"
            "color:#C62828; background-color:#FFEBEE;");
    }

    if (groundMeteoParams) {
        switch (groundMeteoParams->surfaceState()) {
        case GroundMeteoParams::NoData:
            ui->badgeGroundParams->setText(QString::fromUtf8("Нет данных"));
            ui->badgeGroundParams->setStyleSheet(
                "font-size:8pt; font-weight:700; padding:4px 10px; border-radius:10px;"
                "color:#C62828; background-color:#FFEBEE;");
            break;
        case GroundMeteoParams::Stale:
            ui->badgeGroundParams->setText(QString::fromUtf8("Устарели"));
            ui->badgeGroundParams->setStyleSheet(
                "font-size:8pt; font-weight:700; padding:4px 10px; border-radius:10px;"
                "color:#E65100; background-color:#FFF3E0;");
            break;
        case GroundMeteoParams::Fresh:
            ui->badgeGroundParams->setText(QString::fromUtf8("Актуальны"));
            ui->badgeGroundParams->setStyleSheet(
                "font-size:8pt; font-weight:700; padding:4px 10px; border-radius:10px;"
                "color:#0F6B4F; background-color:#E8F5E9;");
            break;
        }
    }
}

bool SourceData::hasMeteo11Bulletin() const
{
    return m_meteo11Dialog && m_meteo11Dialog->isApplied();
}

QJsonObject SourceData::meteo11BulletinJson() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->bulletinJson() : QJsonObject();
}

QDateTime SourceData::meteo11BulletinTime() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->bulletinTime() : QDateTime();
}

QString SourceData::meteo11ValidityPeriod() const
{
    return m_meteo11Dialog ? m_meteo11Dialog->validityPeriod() : QString();
}

void SourceData::resetMeteo11Applied()
{
    if (m_meteo11Dialog)
        m_meteo11Dialog->resetApplied();
}
