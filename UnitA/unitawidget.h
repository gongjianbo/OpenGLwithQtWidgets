#ifndef UNITAWIDGET_H
#define UNITAWIDGET_H

#include <QWidget>

namespace Ui {
class UnitAWidget;
}

class UnitAWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UnitAWidget(QWidget *parent = nullptr);
    ~UnitAWidget();

private:
    Ui::UnitAWidget *ui;
};

#endif // UNITAWIDGET_H
