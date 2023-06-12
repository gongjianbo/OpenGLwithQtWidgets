#include <QApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QDebug>
#include "mainwindow.h"

//解析字符串中的opengl版本号
bool parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor)
{
    bool majorOk = false;
    bool minorOk = false;
    QList<QByteArray> parts = versionString.split(' ');
    if (versionString.startsWith(QByteArrayLiteral("OpenGL ES"))) {
        if (parts.size() >= 3) {
            QList<QByteArray> versionParts = parts.at(2).split('.');
            if (versionParts.size() >= 2) {
                major = versionParts.at(0).toInt(&majorOk);
                minor = versionParts.at(1).toInt(&minorOk);
                // Nexus 6 has "OpenGL ES 3.0V@95.0 (GIT@I86da836d38)"
                if (!minorOk)
                    if (int idx = versionParts.at(1).indexOf('V'))
                        minor = versionParts.at(1).left(idx).toInt(&minorOk);
            } else {
                qWarning("Unrecognized OpenGL ES version");
            }
        } else {
            // If < 3 parts to the name, it is an unrecognised OpenGL ES
            qWarning("Unrecognised OpenGL ES version");
        }
    } else {
        // Not OpenGL ES, but regular OpenGL, the version numbers are first in the string
        QList<QByteArray> versionParts = parts.at(0).split('.');
        if (versionParts.size() >= 2) {
            major = versionParts.at(0).toInt(&majorOk);
            minor = versionParts.at(1).toInt(&minorOk);
        } else {
            qWarning("Unrecognized OpenGL version");
        }
    }

    if (!majorOk || !minorOk)
        qWarning("Unrecognized OpenGL version");
    return (majorOk && minorOk);
}

bool checkVersion(int minMajor, int minMinor)
{
    int major = 0;
    int minor = 0;
    //一般需要先构造一个QGuiApplication有些函数才能调用
    QScreen *screen = qApp->primaryScreen();
    if(screen){
        QOffscreenSurface surface(screen);
        surface.create();
        QOpenGLContext context;
        context.create();
        context.makeCurrent(&surface);

        const GLubyte *glstr = context.functions()->glGetString(GL_VERSION);
        if(glstr){
            QByteArray bytestr = QByteArray(reinterpret_cast<const char*>(glstr));
            qDebug() << "glGetString" << bytestr;
            if(!parseOpenGLVersion(bytestr,major,minor)){
                major = 0;
                minor = 0;
            }
        }
        context.doneCurrent();
        surface.destroy();
    }
    qDebug() << "Native OpenGL version:" << major << minor;
    return (major*10+minor>=minMajor*10+minMinor);
}

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication app(argc, argv);

    //《OpenGL编程指南（原书第九版）》以OpenGL4.5为基础
    qDebug() << "Need OpenGL4.5 desktop context";
    if(!checkVersion(4, 5))
        return -1;

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    //在Qt6设置后显示黑色窗口，或者切换tab崩溃
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    //fmt.setSamples(8);//多重采样
    QSurfaceFormat::setDefaultFormat(fmt);
#endif

    MainWindow w;
    w.show();
    return app.exec();
}
