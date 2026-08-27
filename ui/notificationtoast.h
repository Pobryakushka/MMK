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
// сдвигает остальные элементы.
//
// Тост никогда не висит бесконечно: если время жизни явно не задано,
// оно считается из типа и длины сообщения (defaultDurationFor()).
// Кроме того, уведомление снимается тапом в любом месте карточки
// (отдельной кнопки-крестика для этого нет).
// ============================================================
class NotificationToast : public QWidget
{
    Q_OBJECT
public:
    enum Kind { Info, Success, Error };

    // Специальные значения для параметра autoHideMs.
    enum { AutoDuration = -1, // время жизни считает сам тост (по умолчанию)
           Sticky      = 0 }; // остаётся, пока его не снимут тапом/новым вызовом

    // anchor — страница/диалог, над содержимым которой всплывает уведомление.
    explicit NotificationToast(QWidget *anchor);

    // autoHideMs: AutoDuration — время жизни из defaultDurationFor();
    // Sticky — не скрывать самостоятельно (для этого нужна явная причина:
    // тост всё равно можно снять тапом); >0 — конкретная длительность в мс.
    void showMessage(const QString &text, Kind kind, int autoHideMs = AutoDuration);

    // Как showMessage(), но с дополнительной кнопкой-действием (например,
    // "Открыть папку" после экспорта) — заменяет диалоги с ActionRole-кнопкой.
    // Живёт дольше обычного тоста: у пользователя должно быть время нажать.
    void showMessageWithAction(const QString &text, Kind kind,
                                const QString &actionLabel, std::function<void()> onAction,
                                int autoHideMs = AutoDuration);

    // Сколько миллисекунд показывать сообщение, если время не задано явно:
    // базовая задержка плюс поправка на длину текста, ошибки живут дольше.
    static int defaultDurationFor(Kind kind, const QString &text);

    void dismiss();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QWidget *m_anchor;
    QLabel *m_iconBadge;
    QLabel *m_textLabel;
    QPushButton *m_actionBtn;
    QPropertyAnimation *m_anim;
    QTimer *m_autoHideTimer;
    std::function<void()> m_actionCallback;
    bool m_dismissing = false; // идёт анимация ухода, тост уже "не считается видимым"

    void applyKindStyle(Kind kind);
    void reposition();
    void showMessageInternal(const QString &text, Kind kind, int autoHideMs);
    void finishDismiss();
};

#endif // NOTIFICATIONTOAST_H
