#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>

//帧缓冲
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLFrameBufferQt
        : public QOpenGLWidget
        , protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit GLFrameBufferQt(QWidget *parent = nullptr);
    ~GLFrameBufferQt();

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
    //初始化
    void initScreen();
    void initFbo();
    //初始化或resize之后重置帧缓冲区
    void resetFbo();
    //在paintGL中调用
    void paintScreen();
    void paintFbo();
    //析构时调用
    void freeScreen();
    void freeFbo();

private:
    //【】命名screen表示默认缓冲区相关
    //着色器程序
    QOpenGLShaderProgram screenShaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject screenVao;
    //顶点缓冲
    QOpenGLBuffer screenVbo;

    //【】命名fbo表示自定义缓冲区相关
    //着色器程序
    QOpenGLShaderProgram fboShaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject fboCubeVao, fboPlaneVao;
    //顶点缓冲
    QOpenGLBuffer fboCubeVbo, fboPlaneVbo;
    //纹理
    QOpenGLTexture *fboCubeTexture{ nullptr }, *fboPlaneTexture{ nullptr };
    //帧缓冲
    QOpenGLFramebufferObject *fboBuffer{ nullptr };

    //旋转
    QVector3D rotationAxis;
    QQuaternion rotationQuat;
    //透视投影的fovy参数，视野范围
    float projectionFovy{ 45.0f };
    //鼠标位置
    QPoint mousePos;
    //右键菜单
    QMenu *menu;
    //图像处理算法选择
    int algorithmType{ 0 };
    int drawMode{ 0 };
};
