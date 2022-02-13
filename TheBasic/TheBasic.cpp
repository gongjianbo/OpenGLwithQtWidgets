#include "TheBasic.h"
#include "ui_TheBasic.h"

TheBasic::TheBasic(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TheBasic)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
}

TheBasic::~TheBasic()
{
    delete ui;
}
