#ifndef GLELEMENT_H
#define GLELEMENT_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

//使用缓冲索引对象
class GLElement : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLElement(QWidget *parent = nullptr);
    ~GLElement();

protected:
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    QOpenGLShaderProgram _shaderProgram;
    GLuint _VAO;
    GLuint _VBO;
    GLuint _EBO;
};

#endif // GLELEMENT_H
