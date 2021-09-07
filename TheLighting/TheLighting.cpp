#include "TheLighting.h"
#include "ui_TheLighting.h"

TheLighting::TheLighting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TheLighting)
{
    ui->setupUi(this);
}

TheLighting::~TheLighting()
{
    delete ui;
}
