#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Compatibility>

//OpenGL4.5画三角
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLTriangle
        : public QOpenGLWidget
        , protected QOpenGLFunctions_4_5_Compatibility
{
public:
    explicit GLTriangle(QWidget *parent = nullptr);
    ~GLTriangle();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    void checkShaderError(GLuint id, const QString &type);

private:
    //还没使用Qt的封装
    GLuint shaderProgram{ 0 };
    GLuint vao{ 0 };
    GLuint vbo{ 0 };
};
