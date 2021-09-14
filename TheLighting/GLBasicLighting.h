#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>

//光照基础
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLBasicLighting
        : public QOpenGLWidget
        , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLBasicLighting(QWidget *parent = nullptr);
    ~GLBasicLighting();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

    //鼠标操作,重载Qt的事件处理
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void initShader();

private:
    //着色器程序
    QOpenGLShaderProgram lightingShader,lampShader;
    //顶点数组对象
    QOpenGLVertexArrayObject lightingVao,lampVao;
    //顶点缓冲
    QOpenGLBuffer vbo;

    //
    QVector3D rotationAxis;
    QQuaternion rotationQuat;
    //透视投影的fovy参数，视野范围
    float projectionFovy{ 45.0f };

    //鼠标位置
    QPoint mousePos;

    //旋转
    QTimer timer;
    int rotate{ 0 };
};

