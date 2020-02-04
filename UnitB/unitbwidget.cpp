#include "unitbwidget.h"
#include "ui_unitbwidget.h"

UnitBWidget::UnitBWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UnitBWidget)
{
    ui->setupUi(this);
}

UnitBWidget::~UnitBWidget()
{
    delete ui;
}
