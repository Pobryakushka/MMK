#ifndef NOTIFICATIONTOAST_H
#define NOTIFICATIONTOAST_H

#include <QWidget>
#include <functional>

class QLabel;
class QPushButton;
class QPropertyAnimation;
class QTimer;

// ============================================================
// Плавающее уведомление о статусе/ошибке — карточка со скруглением
// и тенью, выезжающая сверху по центру страницы (тот же визуальный
// язык, что и у "шторки" подключения датчика на главном экране,
// см. MainWindow::m_sensorPopup/setupSensorPopup()).
//
// В отличие от старой инлайн-плашки (QLabel, постоянно занимающий
// место в layout), тост плавает поверх содержимого страницы и не
// сдвигает остальные элементы. Success/Info обновляются на месте без
// повторной анимации; Error остаётся видимым, пока не будет заменён
// следующим вызовом или закрыт кнопкой "×".
// ============================================================
class NotificationToast : public QWidget
{
    Q_OBJECT
public:
    enum Kind { Info, Success, Error };

    // anchor — страница/диалог, над содержимым которой всплывает уведомление.
    explicit NotificationToast(QWidget *anchor);

    // autoHideMs > 0 — уведомление само скроется через это время
    // (используется для разовых подтверждений вроде "Успешно").
    // 0 — остаётся, пока не будет заменено новым вызовом или закрыто.
    void showMessage(const QString &text, Kind kind, int autoHideMs = 0);

    // Как showMessage(), но с дополнительной кнопкой-действием (например,
    // "Открыть папку" после экспорта) — заменяет диалоги с ActionRole-кнопкой.
    void showMessageWithAction(const QString &text, Kind kind,
                                const QString &actionLabel, std::function<void()> onAction);

    void dismiss();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_anchor;
    QLabel *m_iconBadge;
    QLabel *m_textLabel;
    QPushButton *m_actionBtn;
    QPushButton *m_closeBtn;
    QPropertyAnimation *m_anim;
    QTimer *m_autoHideTimer;
    std::function<void()> m_actionCallback;

    void applyKindStyle(Kind kind);
    void reposition();
    void showMessageInternal(const QString &text, Kind kind, int autoHideMs);
};

#endif // NOTIFICATIONTOAST_H
