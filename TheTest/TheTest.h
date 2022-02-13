#pragma once
#include <QWidget>

namespace Ui {
class TheTest;
}

class TheTest : public QWidget
{
    Q_OBJECT

public:
    explicit TheTest(QWidget *parent = nullptr);
    ~TheTest();

private:
    Ui::TheTest *ui;
};
