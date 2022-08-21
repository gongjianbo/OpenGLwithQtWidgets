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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>

//摄像机（观察矩阵）
//QOpenGLWidget窗口上下文
//QOpenGLFunctions访问OpenGL接口，可以不继承作为成员变量使用
class GLCamera
        : public QOpenGLWidget
        , protected QOpenGLFunctions_3_3_Core
{
public:
    explicit GLCamera(QWidget *parent = nullptr);
    ~GLCamera();

protected:
    //【】继承QOpenGLWidget后重写这三个虚函数
    //设置OpenGL资源和状态。在第一次调用resizeGL或paintGL之前被调用一次
    void initializeGL() override;
    //渲染OpenGL场景，每当需要更新小部件时使用
    void paintGL() override;
    //设置OpenGL视口、投影等，每当尺寸大小改变时调用
    void resizeGL(int width, int height) override;

    //按键操作,重载Qt的事件处理
    void keyPressEvent(QKeyEvent *event) override;
    //鼠标操作,重载Qt的事件处理
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void calculateCamera();
    QMatrix4x4 getViewMatrix();

private:
    //着色器程序
    QOpenGLShaderProgram shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject vao;
    //顶点缓冲
    QOpenGLBuffer vbo;
    //纹理（因为不能赋值，所以只能声明为指针）
    QOpenGLTexture *texture1{ nullptr };
    QOpenGLTexture *texture2{ nullptr };
    //定时器，做箱子旋转动画
    QTimer timer;
    int rotate{ 0 };

    //操作View，我这里为了展示没有封装成单独的Camera类
    //Camera Attributes
    QVector3D cameraPos{ 0.0f, 0.0f, 3.0f };
    QVector3D cameraFront{ 0.0f, 0.0f, -1.0f };
    QVector3D cameraUp{ 0.0f, 1.0f, 0.0f };
    QVector3D cameraRight{ 1.0f, 0.0f, 0.0f };
    QVector3D worldUp{ 0.0f, 1.0f, 0.0f };
    //Euler Angles
    //偏航角如果是0.0f,指向的是 x轴正方向，即右方向，所以向里转90度，初始方向指向z轴负方向
    float eulerYaw{ -90.0f }; //偏航角，绕y左右转
    float eulerPitch{ 0.0f }; //俯仰角，绕x上下转
    //Camera options
    float cameraSpeed{ 0.5f }; //移动速度
    float cameraSensitivity{ 0.1f }; //鼠标拖动灵敏度
    float projectionFovy{ 45.0f }; //透视投影的fovy参数，视野范围
    //鼠标位置
    QPoint mousePos;
};
