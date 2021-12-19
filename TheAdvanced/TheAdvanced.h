#pragma once
#include <QWidget>

namespace Ui {
class TheAdvanced;
}

class TheAdvanced : public QWidget
{
    Q_OBJECT
public:
    explicit TheAdvanced(QWidget *parent = nullptr);
    ~TheAdvanced();

private:
    Ui::TheAdvanced *ui;
};

