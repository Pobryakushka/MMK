#include "FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpacing(hSpacing)
    , m_vSpacing(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0))
        delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    return m_items.takeAt(index);
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

QSize FlowLayout::sizeHint() const
{
    // Желаемый размер — «всё в одну строку»: раскладка просит полную ширину
    // ряда и переносит элементы только тогда, когда ей столько не дали.
    // Если вернуть здесь minimumSize() (ширину самого широкого элемента),
    // соседний растягивающийся элемент в родительской раскладке заберёт всё
    // свободное место, и ряд свернётся в столбец даже на широком экране.
    int width = 0;
    int height = 0;
    for (QLayoutItem *item : m_items) {
        const QSize hint = item->sizeHint();
        width += hint.width() + m_hSpacing;
        height = qMax(height, hint.height());
    }
    if (!m_items.isEmpty())
        width -= m_hSpacing;

    const QMargins m = contentsMargins();
    return QSize(qMax(0, width) + m.left() + m.right(),
                 height + m.top() + m.bottom());
}

QSize FlowLayout::minimumSize() const
{
    // Минимум — самый широкий одиночный элемент: уже него строка не бывает.
    QSize size;
    for (QLayoutItem *item : m_items)
        size = size.expandedTo(item->minimumSize());

    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QMargins m = contentsMargins();
    const QRect content = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());

    int x = content.x();
    int y = content.y();
    int lineHeight = 0;

    for (QLayoutItem *item : m_items) {
        const QSize hint = item->sizeHint();

        // Переносим на новую строку, когда элемент не помещается в остаток
        // текущей (но не переносим самый первый элемент строки — иначе при
        // очень узком окне получилась бы пустая строка).
        int next = x + hint.width();
        if (next - 1 > content.right() && lineHeight > 0) {
            x = content.x();
            y = y + lineHeight + m_vSpacing;
            next = x + hint.width();
            lineHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), hint));

        x = next + m_hSpacing;
        lineHeight = qMax(lineHeight, hint.height());
    }

    return y + lineHeight - rect.y() + m.bottom();
}
