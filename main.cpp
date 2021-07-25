#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSurfaceFormat fmt;
    //fmt.setSamples(8);//多重采样
    //《OpenGL编程指南（原书第九版）》以OpenGL4.5为基础
    if(QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL){
        qDebug("Requesting 4.5 context");
        fmt.setVersion(4,5);
        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    w.show();
    return a.exec();
}
