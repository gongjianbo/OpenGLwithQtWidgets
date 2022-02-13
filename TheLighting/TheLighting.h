#pragma once
#include <QWidget>

namespace Ui {
class TheLighting;
}

class TheLighting : public QWidget
{
    Q_OBJECT

public:
    explicit TheLighting(QWidget *parent = nullptr);
    ~TheLighting();

private:
    Ui::TheLighting *ui;
};
