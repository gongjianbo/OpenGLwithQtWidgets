#ifndef GLTRANSFORM_H
#define GLTRANSFORM_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QTimer>

//变换
class GLTransform : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLTransform(QWidget *parent = nullptr);
    ~GLTransform();

protected:
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    //着色器程序
    QOpenGLShaderProgram _shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject _vao;
    //顶点缓冲
    QOpenGLBuffer _vbo;
    //索引缓冲
    QOpenGLBuffer _ebo;
    //纹理（因为不能赋值，所以只能声明为指针）
    QOpenGLTexture *_texture1 = nullptr;
    QOpenGLTexture *_texture2 = nullptr;
    //定时器，做动画效果
    QTimer *_timer=nullptr;
    //旋转角度
    int _rotate=0;
};

#endif // GLTRANSFORM_H
