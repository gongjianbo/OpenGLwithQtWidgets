#include "TheTest.h"
#include "ui_TheTest.h"

TheTest::TheTest(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TheTest)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
}

TheTest::~TheTest()
{
    delete ui;
}
