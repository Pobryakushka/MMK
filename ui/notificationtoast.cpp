#include "notificationtoast.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QEvent>
#include <QColor>

NotificationToast::NotificationToast(QWidget *anchor)
    : QWidget(anchor)
    , m_anchor(anchor)
{
    setObjectName("notificationToast");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "QWidget#notificationToast {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    setGraphicsEffect(shadow);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 12, 10, 12);
    layout->setSpacing(10);

    m_iconBadge = new QLabel(this);
    m_iconBadge->setFixedSize(24, 24);
    m_iconBadge->setAlignment(Qt::AlignCenter);

    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);

    m_actionBtn = new QPushButton(this);
    m_actionBtn->setCursor(Qt::PointingHandCursor);
    m_actionBtn->setFocusPolicy(Qt::NoFocus);
    m_actionBtn->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: #0F6B4F; font-weight: 700; font-size: 10.5pt; padding: 0 6px; }"
        "QPushButton:pressed { color: #0B4F3B; }");
    m_actionBtn->setVisible(false);
    connect(m_actionBtn, &QPushButton::clicked, this, [this]() {
        if (m_actionCallback)
            m_actionCallback();
        dismiss();
    });

    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: #9AA1A6; font-size: 9pt; font-weight: 700; }"
        "QPushButton:pressed { color: #5B6167; }");
    m_closeBtn->setVisible(false);
    connect(m_closeBtn, &QPushButton::clicked, this, &NotificationToast::dismiss);

    layout->addWidget(m_iconBadge, 0, Qt::AlignTop);
    layout->addWidget(m_textLabel, 1);
    layout->addWidget(m_actionBtn, 0, Qt::AlignTop);
    layout->addWidget(m_closeBtn, 0, Qt::AlignTop);

    m_anim = new QPropertyAnimation(this, "pos", this);
    m_anim->setDuration(300);
    m_anim->setEasingCurve(QEasingCurve::OutBack);

    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &NotificationToast::dismiss);

    hide();
    if (m_anchor)
        m_anchor->installEventFilter(this);
}

void NotificationToast::applyKindStyle(Kind kind)
{
    QString badgeBg, glyph, textColor;
    bool showClose = false;

    switch (kind) {
    case Success:
        badgeBg = "#0F6B4F";
        glyph = QStringLiteral("✓"); // ✓
        textColor = "#0F6B4F";
        break;
    case Error:
        badgeBg = "#C62828";
        glyph = "!";
        textColor = "#B71C1C";
        showClose = true;
        break;
    case Info:
    default:
        badgeBg = "#C68A00";
        glyph = "i";
        textColor = "#8D5B00";
        break;
    }

    m_iconBadge->setStyleSheet(QString(
        "background:%1; color:#FFFFFF; border-radius:12px; font-weight:800; font-size:10.5pt;")
        .arg(badgeBg));
    m_iconBadge->setText(glyph);

    m_textLabel->setStyleSheet(QString(
        "color:%1; font-weight:600; font-size:10.5pt; background:transparent; border:none;")
        .arg(textColor));

    m_closeBtn->setVisible(showClose);
}

void NotificationToast::reposition()
{
    if (!m_anchor)
        return;

    const int margin = 20;
    const int maxWidth = qMin(480, m_anchor->width() - 2 * margin);
    setFixedWidth(qMax(220, maxWidth));
    adjustSize();

    const int x = (m_anchor->width() - width()) / 2;
    const int y = 64;
    move(x, y);
}

void NotificationToast::showMessage(const QString &text, Kind kind, int autoHideMs)
{
    m_actionBtn->setVisible(false);
    m_actionCallback = nullptr;
    showMessageInternal(text, kind, autoHideMs);
}

void NotificationToast::showMessageWithAction(const QString &text, Kind kind,
                                               const QString &actionLabel, std::function<void()> onAction)
{
    m_actionBtn->setText(actionLabel);
    m_actionBtn->setVisible(true);
    m_actionCallback = std::move(onAction);
    showMessageInternal(text, kind, 0);
    m_closeBtn->setVisible(true); // всегда можно закрыть, не только через действие
}

void NotificationToast::showMessageInternal(const QString &text, Kind kind, int autoHideMs)
{
    m_autoHideTimer->stop();
    m_anim->stop();

    applyKindStyle(kind);
    m_textLabel->setText(text);

    const bool wasVisible = isVisible();
    reposition();
    const QPoint target = pos();

    raise();
    if (!wasVisible) {
        const QPoint start(target.x(), target.y() - 24);
        move(start);
        show();
        m_anim->setStartValue(start);
        m_anim->setEndValue(target);
        m_anim->start();
    }

    if (autoHideMs > 0)
        m_autoHideTimer->start(autoHideMs);
}

void NotificationToast::dismiss()
{
    m_autoHideTimer->stop();
    m_anim->stop();
    hide();
    m_actionBtn->setVisible(false);
    m_actionCallback = nullptr;
}

bool NotificationToast::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_anchor && event->type() == QEvent::Resize && isVisible())
        reposition();
    return QWidget::eventFilter(watched, event);
}
