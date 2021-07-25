#pragma once
#include <QWidget>

namespace Ui {
class TheBasic;
}

class TheBasic : public QWidget
{
    Q_OBJECT
public:
    explicit TheBasic(QWidget *parent = nullptr);
    ~TheBasic();

private:
    Ui::TheBasic *ui;
};

