#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

// ─────────────────────────────────────────────────────────────────────────
// VirtualKeyboard — переиспользуемая экранная клавиатура для планшета.
// ─────────────────────────────────────────────────────────────────────────

#include <QWidget>
#include <QPointer>
#include <QHash>
#include <QString>
#include <limits>

class QLineEdit;
class QGridLayout;
class QPushButton;
class QLabel;
class QVBoxLayout;

// Ограничения ввода для конкретного поля. Проверяются на каждое нажатие
// клавиши (что можно набрать) и при "Готово" (диапазон значения).

struct VirtualKeyboardConstraints {
    bool   allowNegative   = true;   // разрешён ли знак "-"
    bool   allowDecimal    = true;   // разрешена ли дробная часть
    int    maxDecimals     = -1;     // максимум цифр после разделителя, -1 = не ограничено
    int    maxLength       = -1;     // максимальная длина строки, -1 = не ограничено
    bool   allowModeSwitch = true;   // показывать кнопку переключения цифры/буквы
    double minValue = std::numeric_limits<double>::lowest();
    double maxValue = std::numeric_limits<double>::max();
    bool   clampOnDone = true;       // при "Готово" подрезать значение к [min, max]
                                      // (если false — значение, вышедшее за диапазон,
                                      // просто очищается, чтобы не пропустить некорректные данные)
};

class VirtualKeyboard : public QWidget
{
    Q_OBJECT

public:
    enum class Mode {
        Auto,     // определяется автоматически (по текущему тексту/валидатору поля)
        Numeric,  // компактная цифровая панель: 0-9, ",", "-", ⌫, C, Готово
        Text      // полная раскладка ЙЦУКЕН + цифры + пробел + Shift
    };

    using Constraints = VirtualKeyboardConstraints;

    static VirtualKeyboard* instance();

    // Привязать клавиатуру к полю. target должен существовать всё время,
    // пока он используется; при уничтожении target отвязка происходит
    // автоматически.
    static void attach(QLineEdit *target,
                        Mode mode = Mode::Auto,
                        const Constraints &constraints = Constraints());

    // Отвязать клавиатуру от поля (снимает фильтр событий и запись в реестре).
    static void detach(QLineEdit *target);

    // Немедленно скрыть клавиатуру (например, при закрытии диалога/окна).
    static void hideKeyboard();

    // Построить регулярное выражение, описывающее ДОПУСТИМУЮ строку для
    // заданных ограничений числового поля. Используется и внутри клавиатуры,
    // и может быть переиспользовано как QValidator для полей, которые
    // допускают ручной ввод с физической/системной клавиатуры.
    static QString numericPattern(const Constraints &c);

signals:
    // Эмитится, когда пользователь нажал "Готово" и значение принято.
    void doneEditing(QLineEdit *target);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override; // блокирует "клик мимо" внутри себя

private:
    struct AttachInfo {
        Mode mode = Mode::Auto;
        Constraints constraints;
    };

    explicit VirtualKeyboard(QWidget *parent = nullptr);

    void showFor(QLineEdit *target);
    void rebuildLayout();
    void buildNumericLayout();
    void buildTextLayout();
    void repositionFor(QWidget *target);
    void insertText(const QString &text);
    void backspace();
    void clearField();
    void commitAndClose();
    static QString normalizeNumericText(const QString &input);
    void toggleShift();
    void toggleMode();
    bool candidateAllowed(const QString &candidateText) const;
    Mode effectiveMode() const;
    void updateHint();

    QPointer<QLineEdit> m_target;
    AttachInfo   m_current;
    Mode         m_shownMode = Mode::Numeric;
    bool         m_shift = false;

    QVBoxLayout *m_rootLayout   = nullptr;
    QWidget     *m_keysContainer = nullptr;
    QGridLayout *m_grid = nullptr;
    QLabel      *m_hintLabel = nullptr;

    static QHash<QLineEdit*, AttachInfo> s_registry;
    static VirtualKeyboard *s_instance;
};

#endif // VIRTUALKEYBOARD_H
