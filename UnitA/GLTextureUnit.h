#ifndef GLTEXTUREUNIT_H
#define GLTEXTUREUNIT_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

//纹理单元，本类尝试使用Qt封装得OpenGL相关类
class GLTextureUnit : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
public:
    explicit GLTextureUnit(QWidget *parent = nullptr);
    ~GLTextureUnit();

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
};

#endif // GLTEXTUREUNIT_H
