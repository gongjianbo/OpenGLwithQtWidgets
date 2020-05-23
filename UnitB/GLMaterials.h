#ifndef GLMATERIALS_H
#define GLMATERIALS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

#include <QTimer>

//光照材质
class GLMaterials : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLMaterials(QWidget *parent = nullptr);
    ~GLMaterials();

protected:
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用，暂时没啥用
    //void resizeGL(int width, int height) override;

private:
    //初始化着色器程序
    void initShader();

private:
    //cube 被照射立方体， lamp 灯立方体
    //着色器程序
    QOpenGLShaderProgram _cubeShader,_lampShader;
    //顶点数组对象
    QOpenGLVertexArrayObject _cubeVao,_lampVao;
    //顶点缓冲
    QOpenGLBuffer _vbo;
    //定时qi
    QTimer *_timer;
    //旋转
    float _theAngle=0.0f;
    //反射
    float _theShininess=32.0f;
};

#endif // GLMATERIALS_H
