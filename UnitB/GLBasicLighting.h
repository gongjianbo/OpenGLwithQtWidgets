#ifndef GLBASICLIGHTING_H
#define GLBASICLIGHTING_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimerEvent>

#include "MyCamera.h"

//基础光照
class GLBasicLighting : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLBasicLighting(QWidget *parent = nullptr);
    ~GLBasicLighting();

protected:
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

    //鼠标操作,重载Qt的事件处理
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void initShader();

private:
    //着色器程序
    QOpenGLShaderProgram _lightingShader,_lampShader;
    //顶点数组对象
    QOpenGLVertexArrayObject _lightingVao,_lampVao;
    //顶点缓冲
    QOpenGLBuffer _vbo;
    //view的camera
    MyCamera _camera;
};

#endif // GLBASICLIGHTING_H
