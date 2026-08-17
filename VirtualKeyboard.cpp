#include "VirtualKeyboard.h"

#include <QLineEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <cmath>

QHash<QLineEdit*, VirtualKeyboard::AttachInfo> VirtualKeyboard::s_registry;
VirtualKeyboard* VirtualKeyboard::s_instance = nullptr;

VirtualKeyboard::VirtualKeyboard(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);

    setObjectName("VirtualKeyboard");
    setStyleSheet(
        "#VirtualKeyboard {"
        "  background:#EFF1F1; border:1px solid #B9BFC2; border-radius:14px;"
        "}"
        "QPushButton {"
        "  background:#FFFFFF; border:1px solid #DDE1E3; border-radius:10px;"
        "  font-size:15pt; font-weight:600; color:#1C1F22;"
        "}"
        "QPushButton:pressed { background:#E3F2ED; }"
        "QPushButton#vkAction { background:#E8EBEC; color:#3A4046; }"
        "QPushButton#vkAction:pressed { background:#D7DBDD; }"
        "QPushButton#vkDone { background:#0F6B4F; color:#FFFFFF; }"
        "QPushButton#vkDone:pressed { background:#0B5A41; }"
        "QPushButton#vkShift:checked { background:#0F6B4F; color:#FFFFFF; }"
        "QLabel#vkHint { color:#6B7278; font-size:9pt; padding:2px 6px; }"
    );

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(10, 8, 10, 10);
    m_rootLayout->setSpacing(6);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName("vkHint");
    m_rootLayout->addWidget(m_hintLabel);

    m_keysContainer = new QWidget(this);
    m_grid = new QGridLayout(m_keysContainer);
    m_grid->setSpacing(6);
    m_rootLayout->addWidget(m_keysContainer);

    hide();
}

VirtualKeyboard* VirtualKeyboard::instance()
{
    if (!s_instance)
        s_instance = new VirtualKeyboard(nullptr);
    return s_instance;
}

// ─────────────────────────────────────────────────────────────────────────
// Публичное API
// ─────────────────────────────────────────────────────────────────────────

void VirtualKeyboard::attach(QLineEdit *target, Mode mode, const Constraints &constraints)
{
    if (!target)
        return;

    VirtualKeyboard *kb = instance();

    AttachInfo info;
    info.mode = mode;
    info.constraints = constraints;
    s_registry.insert(target, info);

    // Если поле допускает только числа — заодно навесим QValidator, чтобы
    // ввод с физической/системной клавиатуры (например, USB-клавиатура,
    // подключённая к планшету) тоже подчинялся тем же правилам.
    if (mode == Mode::Numeric) {
        auto *validator = new QRegularExpressionValidator(
            QRegularExpression(numericPattern(constraints)), target);
        target->setValidator(validator);
    }

    target->removeEventFilter(kb);
    target->installEventFilter(kb);

    QObject::connect(target, &QObject::destroyed, kb, [target, kb]() {
        s_registry.remove(target);
        if (kb->m_target == target) {
            kb->m_target = nullptr;
            kb->hide();
        }
    });
}

void VirtualKeyboard::detach(QLineEdit *target)
{
    if (!target)
        return;
    VirtualKeyboard *kb = instance();
    target->removeEventFilter(kb);
    s_registry.remove(target);
    if (kb->m_target == target) {
        kb->hide();
        kb->m_target = nullptr;
    }
}

void VirtualKeyboard::hideKeyboard()
{
    if (s_instance)
        s_instance->hide();
}

QString VirtualKeyboard::numericPattern(const Constraints &c)
{
    // Строит регулярное выражение для ПРОМЕЖУТОЧНЫХ (не обязательно
    // завершённых) состояний ввода числа, например "-", "12,", "0,5" —
    // это осознанно мягче, чем финальная проверка диапазона, которая
    // происходит отдельно, при "Готово"/потере фокуса.
    QString sign = c.allowNegative ? "-?" : "";
    QString intPart = "[0-9]*";
    QString fracPart;
    if (c.allowDecimal) {
        int decimals = c.maxDecimals >= 0 ? c.maxDecimals : -1;
        QString digits = decimals >= 0 ? QString("[0-9]{0,%1}").arg(decimals)
                                        : "[0-9]*";
        fracPart = QString("([\\.,]%1)?").arg(digits);
    }
    return QString("^%1%2%3$").arg(sign, intPart, fracPart);
}

// ─────────────────────────────────────────────────────────────────────────
// Событийный фильтр — показ по фокусу
// ─────────────────────────────────────────────────────────────────────────

bool VirtualKeyboard::eventFilter(QObject *watched, QEvent *event)
{
    if (isVisible() && watched == m_watchedWindow && event->type() == QEvent::MouseButtonPress){
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint pos =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            me->position().toPoint();
#else
            me->pos();
#endif
        const bool onKeyboard = geometry().contains(pos);
        bool onTarget = false;
        if(m_target){
            const QPoint targetTopLeft = m_target->mapTo(m_watchedWindow, QPoint(0, 0));
            onTarget = QRect(targetTopLeft, m_target->size()).contains(pos);
        }
        if (!onKeyboard && !onTarget) {
            commitAndClose();
        }
        return QWidget::eventFilter(watched, event);
    }

    auto *target = qobject_cast<QLineEdit*>(watched);
    if (!target)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::FocusIn) {
        showFor(target);
    } else if (event->type() == QEvent::FocusOut) {
        // Если фокус ушёл не на саму клавиатуру (её кнопки — NoFocus, так
        // что это практически всегда "ушёл на другой виджет приложения"),
        // мягко завершаем редактирование текущего поля.
        if (m_target == target)
            commitAndClose();
    } else if (event->type() == QEvent::Hide || event->type() == QEvent::Destroy) {
        if (m_target == target)
            hide();
    }

    return QWidget::eventFilter(watched, event);
}

void VirtualKeyboard::mousePressEvent(QMouseEvent *event)
{
    // Поглощаем клик по фону клавиатуры, чтобы он не "провалился" в то,
    // что находится под ней, и не снял фокус с поля.
    event->accept();
}

// ─────────────────────────────────────────────────────────────────────────
// Показ / позиционирование
// ─────────────────────────────────────────────────────────────────────────

VirtualKeyboard::Mode VirtualKeyboard::effectiveMode() const
{
    if (m_current.mode != Mode::Auto)
        return m_current.mode;
    // Автоопределение: если у поля стоит числовой валидатор — цифровая
    // раскладка, иначе — полная текстовая.
    if (m_target && qobject_cast<const QRegularExpressionValidator*>(m_target->validator()))
        return Mode::Numeric;
    return Mode::Text;
}

void VirtualKeyboard::showFor(QLineEdit *target)
{
    if (!s_registry.contains(target))
        return; // на всякий случай — поле не было привязано через attach()

    m_target = target;
    m_current = s_registry.value(target);
    m_shift = false;
    m_shownMode = effectiveMode();

    // Клавиатура всегда должна жить в ТОМ ЖЕ top-level окне, что и поле
    // (диалог параметров, главное окно и т.д.) — иначе на встраиваемой
    // платформе без нормального оконного менеджера возможна борьба за
    // фокус между окнами (см. комментарий в конструкторе).
    QWidget *topLevel = target->window();
    if (topLevel && parentWidget() != topLevel) {
        setParent(topLevel); // setParent() скрывает виджет — show() ниже вернёт его
    }

    if (m_watchedWindow != topLevel) {
        if (m_watchedWindow)
            m_watchedWindow->removeEventFilter(this);
        m_watchedWindow = topLevel;
        if (m_watchedWindow)
            m_watchedWindow->installEventFilter(this);
    }

    rebuildLayout();
    updateHint();
    repositionFor(target);
    show();
    raise();
}

void VirtualKeyboard::repositionFor(QWidget *target)
{
    QWidget *topLevel = target->window();
    if (!topLevel)
        return;

    const QRect avail = topLevel->rect();

    if (QLayout *l = layout())
        l->activate();
    const QSize hint = sizeHint();
    const QSize minHint = minimumSizeHint();

    int maxWidth = (m_shownMode == Mode::Text)
        ? qMin(avail.width() * 92 / 100, 980)
        : qMin(avail.width() * 70 / 100, 560);

    int maxHeight = qMin(avail.height() * 55 / 100, m_shownMode == Mode::Text ? 380 : 340);

    int kbWidth  = qMax(minHint.width(), qMin(hint.width(), maxWidth));
    int kbHeight = qMax(minHint.height(), qMin(hint.height(), maxHeight));

    resize(kbWidth, kbHeight);

    // Координаты поля — в системе координат ТОГО ЖЕ topLevel-окна, куда
    // мы только что перепривязали клавиатуру (move() ниже интерпретирует
    // координаты именно в этой системе, т.к. это её родитель).
    const QPoint targetTopLeft = target->mapTo(topLevel, QPoint(0, 0));
    const QRect targetRect(targetTopLeft, target->size());
    const int margin = 10;

    int spaceBelow = avail.bottom() - targetRect.bottom();
    int spaceAbove = targetRect.top() - avail.top();

    int y;
    if (spaceBelow >= kbHeight + margin) {
        y = targetRect.bottom() + margin;
    } else if (spaceAbove >= kbHeight + margin) {
        y = targetRect.top() - kbHeight - margin;
    } else {
        // Ни сверху, ни снизу не хватает места целиком — прижимаем туда,
        // где места больше, и не даём выйти за пределы окна. Перекрытие
        // самого поля ввода в этом случае невозможно (мы всё равно кладём
        // клавиатуру строго выше или строго ниже поля), но клавиатура
        // будет неизбежно у самого края окна.
        y = (spaceBelow >= spaceAbove)
                ? qMax(targetRect.bottom() + margin, avail.bottom() - kbHeight)
                : qMin(targetRect.top() - kbHeight - margin, avail.top());
        y = qBound(avail.top(), y, avail.bottom() - kbHeight);
    }

    int x = targetRect.center().x() - kbWidth / 2;
    if (x + kbWidth > avail.right())
        x = avail.right() - kbWidth;
    if (x < avail.left())
        x = avail.left();

    move(x, y);
}

void VirtualKeyboard::updateHint()
{
    if (m_shownMode != Mode::Numeric) {
        m_hintLabel->setText(m_target ? m_target->objectName() : QString());
        return;
    }
    const auto &c = m_current.constraints;
    QString range;
    if (c.minValue > std::numeric_limits<double>::lowest() &&
        c.maxValue < std::numeric_limits<double>::max()) {
        range = QString("Допустимо: %1…%2")
                    .arg(QString::number(c.minValue, 'f', c.maxDecimals > 0 ? c.maxDecimals : 0))
                    .arg(QString::number(c.maxValue, 'f', c.maxDecimals > 0 ? c.maxDecimals : 0));
    }
    m_hintLabel->setText(range);
}

// ─────────────────────────────────────────────────────────────────────────
// Построение раскладок
// ─────────────────────────────────────────────────────────────────────────

void VirtualKeyboard::rebuildLayout()
{
    QLayoutItem *child;
    while ((child = m_grid->takeAt(0)) != nullptr) {
        if (QWidget *w = child->widget())
            w->deleteLater();
        delete child;
    }

    if (m_shownMode == Mode::Numeric)
        buildNumericLayout();
    else
        buildTextLayout();
}

void VirtualKeyboard::buildNumericLayout()
{
    const auto &c = m_current.constraints;
    auto mkKey = [this](const QString &label, auto slotFn, const char *objName = nullptr) {
        auto *btn = new QPushButton(label, m_keysContainer);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setMinimumSize(64, 52);
        if (objName)
            btn->setObjectName(objName);
        connect(btn, &QPushButton::clicked, this, slotFn);
        return btn;
    };

    const char *digits[4][3] = {
        {"7", "8", "9"},
        {"4", "5", "6"},
        {"1", "2", "3"},
        {c.allowNegative ? "-" : "", "0", c.allowDecimal ? "." : ""}
    };

    for (int r = 0; r < 4; ++r) {
        for (int col = 0; col < 3; ++col) {
            QString label = QString::fromUtf8(digits[r][col]);
            if (label.isEmpty()) {
                m_grid->addWidget(new QWidget(m_keysContainer), r, col);
                continue;
            }
            auto *btn = mkKey(label, [this, label]() { insertText(label); });
            m_grid->addWidget(btn, r, col);
        }
    }

    auto *btnBack = mkKey(QString::fromUtf8("⌫"), &VirtualKeyboard::backspace, "vkAction");
    m_grid->addWidget(btnBack, 0, 3);

    auto *btnClear = mkKey(QString::fromUtf8("C"), &VirtualKeyboard::clearField, "vkAction");
    m_grid->addWidget(btnClear, 1, 3);

    auto *btnDone = mkKey(QString::fromUtf8("Готово"), &VirtualKeyboard::commitAndClose, "vkDone");
    m_grid->addWidget(btnDone, 2, 3, 2, 1);

    if (c.allowModeSwitch) {
        auto *btnAbc = mkKey(QString::fromUtf8("АБВ"), &VirtualKeyboard::toggleMode, "vkAction");
        m_grid->addWidget(btnAbc, 3, 3);
    }

    for (int col = 0; col < 4; ++col)
        m_grid->setColumnStretch(col, 1);
    for (int row = 0; row < 4; ++row)
        m_grid->setRowStretch(row, 1);
}

void VirtualKeyboard::buildTextLayout()
{
    auto mkKey = [this](const QString &label, auto slotFn, const char *objName = nullptr) {
        auto *btn = new QPushButton(label, m_keysContainer);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setMinimumSize(48, 48);
        if (objName)
            btn->setObjectName(objName);
        connect(btn, &QPushButton::clicked, this, slotFn);
        return btn;
    };

    static const QStringList row0 = {"1","2","3","4","5","6","7","8","9","0"};
    static const QStringList row1 = {"й","ц","у","к","е","н","г","ш","щ","з","х","ъ"};
    static const QStringList row2 = {"ф","ы","в","а","п","р","о","л","д","ж","э"};
    static const QStringList row3 = {"я","ч","с","м","и","т","ь","б","ю",","};

    int r = 0;
    for (int i = 0; i < row0.size(); ++i) {
        auto *btn = mkKey(row0[i], [this, i]() { insertText(row0[i]); });
        m_grid->addWidget(btn, r, i);
    }
    auto *btnBack = mkKey(QString::fromUtf8("⌫"), &VirtualKeyboard::backspace, "vkAction");
    m_grid->addWidget(btnBack, r, row0.size(), 1, 2);

    r = 1;
    for (int i = 0; i < row1.size(); ++i) {
        QString ch = m_shift ? row1[i].toUpper() : row1[i];
        auto *btn = mkKey(ch, [this, i]() {
            insertText(m_shift ? row1[i].toUpper() : row1[i]);
        });
        m_grid->addWidget(btn, r, i);
    }

    r = 2;
    for (int i = 0; i < row2.size(); ++i) {
        QString ch = m_shift ? row2[i].toUpper() : row2[i];
        auto *btn = mkKey(ch, [this, i]() {
            insertText(m_shift ? row2[i].toUpper() : row2[i]);
        });
        m_grid->addWidget(btn, r, i);
    }

    r = 3;
    auto *btnShift = mkKey(QString::fromUtf8("⇧"), &VirtualKeyboard::toggleShift, "vkShift");
    btnShift->setCheckable(true);
    btnShift->setChecked(m_shift);
    m_grid->addWidget(btnShift, r, 0);
    for (int i = 0; i < row3.size(); ++i) {
        QString ch = m_shift ? row3[i].toUpper() : row3[i];
        auto *btn = mkKey(ch, [this, i]() {
            insertText(m_shift ? row3[i].toUpper() : row3[i]);
        });
        m_grid->addWidget(btn, r, i + 1);
    }

    r = 4;
    if (m_current.constraints.allowModeSwitch) {
        auto *btn123 = mkKey(QString::fromUtf8("123"), &VirtualKeyboard::toggleMode, "vkAction");
        m_grid->addWidget(btn123, r, 0, 1, 2);
    }
    auto *btnSpace = new QPushButton(QString::fromUtf8("Пробел"), m_keysContainer);
    btnSpace->setFocusPolicy(Qt::NoFocus);
    btnSpace->setObjectName("vkAction");
    btnSpace->setMinimumSize(48, 48);
    connect(btnSpace, &QPushButton::clicked, this, [this]() { insertText(" "); });
    m_grid->addWidget(btnSpace, r, 2, 1, 6);
    auto *btnDone = mkKey(QString::fromUtf8("Готово"), &VirtualKeyboard::commitAndClose, "vkDone");
    m_grid->addWidget(btnDone, r, 8, 1, 4);

    for (int col = 0; col < 12; ++col)
        m_grid->setColumnStretch(col, 1);
    for (int row = 0; row < 5; ++row)
        m_grid->setRowStretch(row, 1);
}

// ─────────────────────────────────────────────────────────────────────────
// Ввод / валидация
// ─────────────────────────────────────────────────────────────────────────

bool VirtualKeyboard::candidateAllowed(const QString &candidateText) const
{
    const auto &c = m_current.constraints;

    if (c.maxLength >= 0 && candidateText.length() > c.maxLength)
        return false;

    if (m_shownMode != Mode::Numeric)
        return true; // текстовый режим — единственное ограничение это длина

    QRegularExpression pattern(numericPattern(c));
    QRegularExpressionMatch m = pattern.match(candidateText);
    return m.hasMatch();
}

void VirtualKeyboard::insertText(const QString &text)
{
    if (!m_target)
        return;
    const int pos = m_target->cursorPosition();
    QString current = m_target->text();
    QString candidate = current.left(pos) + text + current.mid(pos);

    if (!candidateAllowed(candidate))
        return; // тихо игнорируем недопустимое нажатие — простая и понятная защита от ошибок

    m_target->insert(text);
}

void VirtualKeyboard::backspace()
{
    if (!m_target)
        return;
    if (m_target->hasSelectedText()) {
        m_target->del();
        return;
    }
    const int pos = m_target->cursorPosition();
    if (pos == 0)
        return;
    QString current = m_target->text();
    current.remove(pos - 1, 1);
    m_target->setText(current);
    m_target->setCursorPosition(pos - 1);
}

void VirtualKeyboard::clearField()
{
    if (!m_target)
        return;
    m_target->clear();
}

void VirtualKeyboard::toggleShift()
{
    m_shift = !m_shift;
    rebuildLayout();
}

void VirtualKeyboard::toggleMode()
{
    m_shownMode = (m_shownMode == Mode::Numeric) ? Mode::Text : Mode::Numeric;
    m_shift = false;
    rebuildLayout();
    updateHint();
    if (m_target)
        repositionFor(m_target);
}

void VirtualKeyboard::commitAndClose()
{
    QLineEdit *target = m_target;
    if (!target) {
        hide();
        return;
    }

    if (m_current.mode == Mode::Numeric ||
        (m_current.mode == Mode::Auto && m_shownMode == Mode::Numeric)) {
        const auto &c = m_current.constraints;
        QString text = target->text();
        text.replace(',', '.');
        text = normalizeNumericText(text);

        if (!text.isEmpty()) {
            bool ok = false;
            double value = text.toDouble(&ok);
            if (ok) {
                if (value < c.minValue || value > c.maxValue) {
                    if (c.clampOnDone) {
                        value = qBound(c.minValue, value, c.maxValue);
                        int decimals = c.maxDecimals >= 0 ? c.maxDecimals : 1;
                        text = QString::number(value, 'f', decimals);
                    } else {
                        text.clear();
                    }
                }
            } else {
                text.clear(); // осталась незавершённая запись вроде "-" — считаем полем пустым
            }
        }

        target->setText(text);
    }

    hide();
    emit doneEditing(target);
}

QString VirtualKeyboard::normalizeNumericText(const QString &input)
{
    if (input.isEmpty())
        return input;

    QString text = input;
    if (text.endsWith('.'))
        text.chop(1);

    const bool negative = text.startsWith('-');
    QString body = negative ? text.mid(1) : text;

    if (body.isEmpty() || body == ".")
        return QString();

    const int dotPos = body.indexOf('.');
    QString intPart = (dotPos >= 0) ? body.left(dotPos) : body;
    QString fracPart = (dotPos >= 0) ? body.mid(dotPos) : QString();

    if (intPart.isEmpty())
        intPart = "0";

    int firstNonZero = 0;
    while (firstNonZero < intPart.length() - 1 && intPart.at(firstNonZero) == '0')
        ++firstNonZero;
    intPart = intPart.mid(firstNonZero);

    text = intPart + fracPart;

    bool ok = false;
    const double value = text.toDouble(&ok);
    const bool keepSign = negative && (!ok || value != 0.0);

    return keepSign ? ('-' + text) : text;
}