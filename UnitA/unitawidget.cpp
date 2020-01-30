#include "unitawidget.h"
#include "ui_unitawidget.h"

UnitAWidget::UnitAWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UnitAWidget)
{
    ui->setupUi(this);
    //在ui中提升类，放入自定义的glwidget
}

UnitAWidget::~UnitAWidget()
{
    delete ui;
}
