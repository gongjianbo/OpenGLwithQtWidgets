#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
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

//射线拾取练习
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class MyRayPick
        : public QOpenGLWidget
        , protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit MyRayPick(QWidget *parent = nullptr);
    ~MyRayPick();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

    //按键操作,重载Qt的事件处理
    //void keyPressEvent(QKeyEvent *event) override;
    //鼠标操作,重载Qt的事件处理
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    //添加射线
    void appendRay(const QPointF &pos);
    QVector3D calcVec3(float x, float y, float z);

private:
    //【1】射线
    QOpenGLShaderProgram rayProgram;
    QOpenGLVertexArrayObject rayVao;
    QOpenGLBuffer rayVbo;
    QVector<QVector3D> rayData;
    //【2】立方体
    QOpenGLShaderProgram boxProgram;
    QOpenGLVertexArrayObject boxVao;
    QOpenGLBuffer boxVbo;
    //
    QVector3D rotationAxis;
    QQuaternion rotationQuat;
    //透视投影的fovy参数，视野范围
    float projectionFovy{ 45.0f };

    //鼠标位置
    QPoint mousePos;
};
