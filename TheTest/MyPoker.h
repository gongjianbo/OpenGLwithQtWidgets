#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTimer>

//扑克反转练习
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class MyPoker
        : public QOpenGLWidget
        , protected QOpenGLFunctions_3_3_Core
{
public:
    explicit MyPoker(QWidget *parent = nullptr);
    ~MyPoker();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    //着色器程序
    QOpenGLShaderProgram shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject vao;
    //顶点缓冲
    QOpenGLBuffer vbo;

    //纹理
    QOpenGLTexture *texture{ nullptr };
    //定时器，做动画效果
    QTimer timer{ nullptr };
    //旋转角度
    int rotate{ 0 };
};
