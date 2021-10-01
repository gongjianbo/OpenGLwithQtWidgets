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
#include <QTimer>

//多光源
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLMultipleLights
        : public QOpenGLWidget
        , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLMultipleLights(QWidget *parent = nullptr);
    ~GLMultipleLights();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

private:
    void initShader();
    QOpenGLTexture *initTexture(const QString &imgpath);

private:
    //着色器程序
    QOpenGLShaderProgram lightingShader,lampShader;
    //顶点数组对象
    QOpenGLVertexArrayObject lightingVao,lampVao;
    //顶点缓冲
    QOpenGLBuffer vbo;
    //纹理
    QOpenGLTexture *diffuseMap{ nullptr };
    QOpenGLTexture *specularMap{ nullptr };
    //
    QTimer timer;
    int rotate{ 0 };
};

