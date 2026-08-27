#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QList>

/**
 * @brief Раскладка, переносящая элементы на следующую строку по ширине.
 *
 * Нужна там, где в макете стоит flex-wrap: строка вкладок архива и ряды
 * кнопок-переключателей Метео-11. QHBoxLayout в узком окне (планшет 1200x1920
 * при масштабе 150% даёт всего 800 логических точек по ширине) сжимает кнопки
 * до нечитаемого состояния и обрезает крайние; FlowLayout вместо этого
 * переносит их вниз, сохраняя исходный размер.
 *
 * Элементы не растягиваются: каждый занимает свой sizeHint(). Высота
 * раскладки зависит от ширины, поэтому реализованы hasHeightForWidth() и
 * heightForWidth().
 */
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent = nullptr,
                        int margin = 0, int hSpacing = 6, int vSpacing = 6);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;

private:
    // Раскладывает элементы по строкам; при testOnly ничего не двигает,
    // а только возвращает нужную высоту — этим пользуется heightForWidth().
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> m_items;
    int m_hSpacing;
    int m_vSpacing;
};

#endif // FLOWLAYOUT_H
