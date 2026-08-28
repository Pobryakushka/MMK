#include "SourceData.h"
#include "ui_SourceData.h"
#include "Meteo11.h"
#include "sensors/GroundMeteoParams.h"
#include "ui/ScreenTheme.h"
#include <QShowEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────
// SourceDataTile
// ─────────────────────────────────────────────────────────────────────────

SourceDataTile::SourceDataTile(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMinimumHeight(72);
}

void SourceDataTile::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void SourceDataTile::setDescription(const QString &description)
{
    m_description = description;
    update();
}

void SourceDataTile::setBadge(const QString &text, const QColor &fg, const QColor &bg)
{
    m_badgeText = text;
    m_badgeFg = fg;
    m_badgeBg = bg;
    update();
}

QSize SourceDataTile::sizeHint() const
{
    return QSize(400, 72);
}

void SourceDataTile::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Фон карточки: белый обычно, чуть темнее при нажатии (isDown() —
    // штатное состояние QAbstractButton, ничего вручную отслеживать не надо)
    const QRectF r = rect().adjusted(0, 0, -1, -1);
    p.setPen(QPen(QColor("#DDE1E3"), 1));
    p.setBrush(isDown() ? QColor("#F0F1F2") : QColor("#FFFFFF"));
    p.drawRoundedRect(r, 18, 18);

    const int leftPad = 26;
    const int rightPad = 22;
    const int chevronWidth = 18;

    // Бейдж — считаем его геометрию первым, он "заякорен" от правого края
    QFont badgeFont = font();
    badgeFont.setPointSizeF(8.0);
    badgeFont.setBold(true);
    QFontMetrics bfm(badgeFont);
    const int badgePadH = 10;
    const int badgeH = 22;
    const int badgeTextW = m_badgeText.isEmpty() ? 0 : bfm.horizontalAdvance(m_badgeText);
    const int badgeW = m_badgeText.isEmpty() ? 0 : badgeTextW + badgePadH * 2;
    const int badgeRight = width() - rightPad - chevronWidth - 10;
    const QRectF badgeRect(badgeRight - badgeW, (height() - badgeH) / 2.0, badgeW, badgeH);

    if (!m_badgeText.isEmpty()) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_badgeBg);
        p.drawRoundedRect(badgeRect, badgeH / 2.0, badgeH / 2.0);
        p.setPen(m_badgeFg);
        p.setFont(badgeFont);
        p.drawText(badgeRect, Qt::AlignCenter, m_badgeText);
    }

    // Шеврон
    QFont chevronFont = font();
    chevronFont.setPointSizeF(12);
    p.setFont(chevronFont);
    p.setPen(QColor("#6B7278"));
    const QRectF chevronRect(width() - rightPad - chevronWidth, 0, chevronWidth, height());
    p.drawText(chevronRect, Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8("\u203A"));

    // Текстовая колонка (заголовок + описание), занимает всё оставшееся
    // место слева от бейджа
    const qreal textRight = badgeRect.left() - 14;
    const QRectF textRect(leftPad, 0, textRight - leftPad, height());

    QFont titleFont = font();
    titleFont.setPointSizeF(13);
    titleFont.setBold(true);
    QFontMetrics tfm(titleFont);

    QFont descFont = font();
    descFont.setPointSizeF(9);
    QFontMetrics dfm(descFont);

    const int titleH = tfm.height();
    const int gap = 2;
    const int descH = dfm.height();
    const int totalTextH = titleH + gap + descH;
    const qreal textTop = (height() - totalTextH) / 2.0;

    p.setPen(QColor("#1C1F22"));
    p.setFont(titleFont);
    p.drawText(QRectF(textRect.left(), textTop, textRect.width(), titleH),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    p.setPen(QColor("#6B7278"));
    p.setFont(descFont);
    p.drawText(QRectF(textRect.left(), textTop + titleH + gap, textRect.width(), descH),
               Qt::AlignLeft | Qt::AlignVCenter, m_description);
}

// ─────────────────────────────────────────────────────────────────────────
// SourceData
// ─────────────────────────────────────────────────────────────────────────

SourceData::SourceData(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SourceData)
    , groundMeteoParams(nullptr)
    , m_meteo11Dialog(nullptr)
{
    ui->setupUi(this);

    // Вид экрана в стиле «Архива измерений» (фон, кнопки, поля). Роль
    // кнопки помечаем ДО применения темы — селектор [nav] сработает сразу.
    setupArchiveBackButton(ui->btnSourceDataBack);
    applyArchiveScreenTheme(this);

    ui->rowMeteo11->setTitle(QString::fromUtf8("Бюллетень «Метео-11»"));
    ui->rowMeteo11->setDescription(QString::fromUtf8("Ввод/просмотр закодированного бюллетеня"));

    ui->rowGroundParams->setTitle(QString::fromUtf8("Параметры приземного слоя"));
    ui->rowGroundParams->setDescription(QString::fromUtf8("Ветер, давление, влажность, температура у земли"));

    groundMeteoParams = new GroundMeteoParams(this);
    qDebug() << "GroundMeteoParams instance created in SourceData";

    // Создаём ПОСТОЯННЫЙ экземпляр Meteo11 — данные не теряются
    // между открытиями страницы и сохраняются до нажатия «Пуск».
    // Meteo11 — обычный QWidget, встраивается в стек MainWindow через
    // meteo11Widget(), а не показывается отдельным окном.
    m_meteo11Dialog = new Meteo11(this);

    // Обычные сигналы QAbstractButton::clicked — тот же механизм, что и у
    // btnAlgLandingCalc в AlgorithmsCalculation, никакой ручной обработки
    // мышиных событий.
    connect(ui->rowMeteo11, &QAbstractButton::clicked, this, [this]() {
        emit openMeteo11Requested();
    });

    connect(ui->rowGroundParams, &QAbstractButton::clicked, this, [this]() {
        emit openGroundParamsRequested();
    });

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

void SourceData::updateBadges()
{
    if (hasMeteo11Bulletin())
        ui->rowMeteo11->setBadge(QString::fromUtf8("Загружен"), QColor("#0B5A41"), QColor("#E4F1EC"));
    else
        ui->rowMeteo11->setBadge(QString::fromUtf8("Не загружен"), QColor("#B3261E"), QColor("#FBE4E4"));

    if (groundMeteoParams) {
        switch (groundMeteoParams->surfaceState()) {
        case GroundMeteoParams::NoData:
            ui->rowGroundParams->setBadge(QString::fromUtf8("Нет данных"), QColor("#B3261E"), QColor("#FBE4E4"));
            break;
        case GroundMeteoParams::Stale:
            ui->rowGroundParams->setBadge(QString::fromUtf8("Устарели"), QColor("#8A6100"), QColor("#FFF4DC"));
            break;
        case GroundMeteoParams::Fresh:
            ui->rowGroundParams->setBadge(QString::fromUtf8("Актуальны"), QColor("#0B5A41"), QColor("#E4F1EC"));
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
