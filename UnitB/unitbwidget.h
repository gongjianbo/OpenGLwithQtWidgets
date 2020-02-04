#ifndef UNITBWIDGET_H
#define UNITBWIDGET_H

#include <QWidget>

namespace Ui {
class UnitBWidget;
}

class UnitBWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UnitBWidget(QWidget *parent = nullptr);
    ~UnitBWidget();

private:
    Ui::UnitBWidget *ui;
};

#endif // UNITBWIDGET_H
