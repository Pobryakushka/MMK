#include "ui/widgets/notificationtoast.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QColor>

NotificationToast::NotificationToast(QWidget *anchor)
    : QWidget(anchor)
    , m_anchor(anchor)
{
    setObjectName("notificationToast");
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor); // вся карточка — область "закрыть"
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
    // Метки не перехватывают тап — он должен доходить до карточки целиком.
    m_iconBadge->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

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

    layout->addWidget(m_iconBadge, 0, Qt::AlignTop);
    layout->addWidget(m_textLabel, 1);
    layout->addWidget(m_actionBtn, 0, Qt::AlignTop);

    m_anim = new QPropertyAnimation(this, "pos", this);
    m_anim->setDuration(300);
    m_anim->setEasingCurve(QEasingCurve::OutBack);
    // finished() приходит только при естественном завершении: stop() его не
    // шлёт, поэтому перезапуск анимации новым сообщением не спрячет тост.
    connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
        if (m_dismissing)
            finishDismiss();
    });

    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &NotificationToast::dismiss);

    hide();
    if (m_anchor)
        m_anchor->installEventFilter(this);
}

int NotificationToast::defaultDurationFor(Kind kind, const QString &text)
{
    // Комфортная скорость чтения — примерно 17 символов в секунду; плюс
    // время на то, чтобы заметить всплывшую карточку.
    const int readMs = 1500 + text.length() * 60;

    switch (kind) {
    case Error:
        // Ошибку нужно успеть прочитать и осмыслить — держим заметно дольше.
        return qBound(6000, readMs, 15000);
    case Success:
    case Info:
    default:
        return qBound(3000, readMs, 8000);
    }
}

void NotificationToast::applyKindStyle(Kind kind)
{
    QString badgeBg, glyph, textColor;

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
                                               const QString &actionLabel, std::function<void()> onAction,
                                               int autoHideMs)
{
    m_actionBtn->setText(actionLabel);
    m_actionBtn->setVisible(true);
    m_actionCallback = std::move(onAction);
    // Кнопку действия надо успеть нажать, поэтому по умолчанию такой тост
    // живёт дольше обычного.
    if (autoHideMs == AutoDuration)
        autoHideMs = defaultDurationFor(kind, text) + 6000;
    showMessageInternal(text, kind, autoHideMs);
}

void NotificationToast::showMessageInternal(const QString &text, Kind kind, int autoHideMs)
{
    m_autoHideTimer->stop();
    m_anim->stop();

    // Новое сообщение отменяет начатый уход: карточка остаётся на экране.
    // Здесь важен именно isHidden() (собственное состояние тоста), а не
    // isVisible(): страница-владелец может быть временно скрыта в QStackedWidget.
    const bool wasVisible = !isHidden() && !m_dismissing;
    m_dismissing = false;

    applyKindStyle(kind);
    m_textLabel->setText(text);

    reposition();
    const QPoint target = pos();

    raise();
    if (!wasVisible) {
        const QPoint start(target.x(), target.y() - 24);
        move(start);
        show();
        m_anim->setEasingCurve(QEasingCurve::OutBack);
        m_anim->setStartValue(start);
        m_anim->setEndValue(target);
        m_anim->start();
    }

    if (autoHideMs == AutoDuration)
        autoHideMs = defaultDurationFor(kind, text);
    if (autoHideMs > 0)
        m_autoHideTimer->start(autoHideMs);
}

void NotificationToast::dismiss()
{
    m_autoHideTimer->stop();

    if (isHidden() || m_dismissing)
        return;

    // Уходим обратно вверх — так же, как появлялись.
    m_anim->stop();
    m_dismissing = true;
    m_anim->setEasingCurve(QEasingCurve::InCubic);
    m_anim->setStartValue(pos());
    m_anim->setEndValue(QPoint(x(), y() - 28));
    m_anim->start();
}

void NotificationToast::finishDismiss()
{
    m_dismissing = false;
    hide();
    m_actionBtn->setVisible(false);
    m_actionCallback = nullptr;
}

void NotificationToast::mousePressEvent(QMouseEvent *event)
{
    // Тап в любом месте карточки убирает уведомление.
    Q_UNUSED(event)
    dismiss();
}

bool NotificationToast::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_anchor && event->type() == QEvent::Resize && !isHidden() && !m_dismissing)
        reposition();
    return QWidget::eventFilter(watched, event);
}
