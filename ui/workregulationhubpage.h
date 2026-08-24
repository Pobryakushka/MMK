#ifndef WORKREGULATIONHUBPAGE_H
#define WORKREGULATIONHUBPAGE_H

#include <QWidget>

namespace Ui {
class WorkRegulationHubPage;
}

// ============================================================
// Страница-хаб «Регламентные работы» — список из двух пунктов
// (КО, ТО-2), при выборе открывает соответствующую отдельную
// встроенную страницу. Сама ничего не делает с АМС — только
// навигация, поэтому AMSHandler ей не нужен.
// ============================================================
class WorkRegulationHubPage : public QWidget
{
    Q_OBJECT
public:
    explicit WorkRegulationHubPage(QWidget *parent = nullptr);
    ~WorkRegulationHubPage() override;

signals:
    void backRequested();
    void openInspectionRequested();   // «Контрольный осмотр (КО)»
    void openAngleCheckRequested();   // «Проверка установки углов (ТО-2)»

private:
    Ui::WorkRegulationHubPage *ui;
};

#endif // WORKREGULATIONHUBPAGE_H
