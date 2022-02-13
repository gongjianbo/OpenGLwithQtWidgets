#include "TheLighting.h"
#include "ui_TheLighting.h"

TheLighting::TheLighting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TheLighting)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
}

TheLighting::~TheLighting()
{
    delete ui;
}
