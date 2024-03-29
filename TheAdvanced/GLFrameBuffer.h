#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

//帧缓冲
//这里主要是测试gl的接口调用，逻辑就是贴图到帧缓冲1，
//再将帧缓冲1作为纹理贴到当前渲染的帧缓冲2
//本文参考：
//他有些地方是错误，比如QOpenGLWidget是不能绑定到ID为0的帧缓冲，
//QOpenGLWidget有一个默认的帧缓冲区，
//defaultFramebufferObject()能获取到它的帧缓存ID
//https://blog.csdn.net/z136411501/article/details/83588308
//https://www.bilibili.com/read/cv11765941/
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLFrameBuffer
        : public QOpenGLWidget
        , protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit GLFrameBuffer(QWidget *parent = nullptr);
    ~GLFrameBuffer();

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
    //索引缓冲
    QOpenGLBuffer ebo;
    //纹理
    QOpenGLTexture *texture{ nullptr };
    //帧缓冲
    unsigned int framebuffer;
    unsigned int texturebuffer;
    //投影矩阵
    QMatrix4x4 projection;
};
