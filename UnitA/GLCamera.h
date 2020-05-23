#ifndef GLCAMERA_H
#define GLCAMERA_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

//摄像机
class GLCamera : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit GLCamera(QWidget *parent = nullptr);
    ~GLCamera();

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
    void calculateCamera();
    QMatrix4x4 getViewMatrix();

private:
    //着色器程序
    QOpenGLShaderProgram _shaderProgram;
    //顶点数组对象
    QOpenGLVertexArrayObject _vao;
    //顶点缓冲
    QOpenGLBuffer _vbo;
    //纹理（因为不能赋值，所以只能声明为指针）
    QOpenGLTexture *_texture1 = nullptr;
    QOpenGLTexture *_texture2 = nullptr;
    //定时器，做箱子旋转动画
    QTimer *_timer=nullptr;
    //箱子旋转角度
    int _rotate=0;

    //操作View，我这里为了展示没有封装成单独的Camera类
    //Camera Attributes
    QVector3D _cameraPosition{0.0f,0.0f,3.0f};
    QVector3D _cameraFront{-_cameraPosition};
    QVector3D _cameraUp{0.0f,1.0f,0.0f};
    QVector3D _cameraRight{};
    QVector3D _cameraWorldUp{_cameraUp};
    // Euler Angles
    //偏航角如果是0.0f,指向的是 x轴正方向，即右方向，所以向里转90度，初始方向指向z轴负方向
    //（这里有个问题，教程是90，但是算出来整体向右偏移了）
    float _Yaw=-89.5f; //x偏航角
    float _Pitch=0.0f; //y俯仰角
    // Camera options
    float _Speed=2.0f;
    float _Sensitivity=0.01f;
    float _Zoom=45.0f; //缩放加限制范围
    //
    QPoint _mousePos;
};

#endif // GLCAMERA_H
