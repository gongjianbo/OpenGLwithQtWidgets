#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>

//甜甜圈-环面
//参考OpenGL超级宝典第三章Demo
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class MyTorus
        : public QOpenGLWidget
        , protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit MyTorus(QWidget *parent = nullptr);
    ~MyTorus();

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
    //使用Qt提供的便捷类
    QOpenGLShaderProgram shaderProgram;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    //顶点数据
    QVector<QVector3D> vertex;
    //
    QVector3D rotationAxis;
    QQuaternion rotationQuat;
    //透视投影的fovy参数，视野范围
    float projectionFovy{45.0f};

    //鼠标位置
    QPoint mousePos;
    //右键菜单
    QMenu *menu;
    bool enableDepthTest{false};
    bool enableCullBackFace{false};
    int drawMode{0};
};
