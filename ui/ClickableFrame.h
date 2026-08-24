#pragma once
#include <QFrame>

// ─────────────────────────────────────────────────────────────────────────
// ClickableFrame — обычный QFrame с сигналом clicked(). Реализовано через
// прямое переопределение mousePressEvent/mouseReleaseEvent (НЕ через
// installEventFilter — на этой платформе перехват мышиных событий через
// eventFilter уже приводил к зависанию приложения в другом месте интерфейса,
// поэтому для кликабельных элементов используем только штатный, надёжный
// путь — тот же, что использует сам Qt внутри QAbstractButton).
//
// Вынесен из mainwindow.h в отдельный файл, т.к. теперь используется не
// только в MainWindow (плашка готовности приземных данных), но и в
// WorkRegulationPage (заголовки секций аккордеона).
// ─────────────────────────────────────────────────────────────────────────
class ClickableFrame : public QFrame
{
    Q_OBJECT
public:
    explicit ClickableFrame(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_pressed = false;
};
