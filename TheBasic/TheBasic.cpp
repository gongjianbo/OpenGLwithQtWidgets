#include "TheBasic.h"
#include "ui_TheBasic.h"

TheBasic::TheBasic(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TheBasic)
{
    ui->setupUi(this);
}

TheBasic::~TheBasic()
{
    delete ui;
}
