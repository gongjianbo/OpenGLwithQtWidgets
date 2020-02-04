#ifndef MYCAMERA_H
#define MYCAMERA_H

#include <QObject>
#include <QMatrix4x4>

// Default camera values
static const float YAW         = -89.5f;
static const float PITCH       =  0.0f;
static const float SPEED       =  2.5f;
static const float SENSITIVITY =  0.02f;
static const float ZOOM        =  90.0f;

//移植LearnOpenGL教程的Cmaera类（入门章节）
//没有修改变量大小写
//An abstract camera class that processes input and calculates the
//corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class MyCamera : public QObject
{
    Q_OBJECT
public:
    explicit MyCamera(QObject *parent = nullptr);
    // Constructor with vectors
    MyCamera(const QVector3D &position,const QVector3D &up,
             float yaw, float pitch,QObject *parent = nullptr);
    ~MyCamera();

    QMatrix4x4 getViewMatrix() const;
    float getZoom() const;

    void keyPress(int key);
    void mousePress(QPoint pos);
    void mouseMove(QPoint pos);
    void mouseRelease(QPoint pos);
    void mouseWheel(int delta);

private:
    void updateCameraVectors();

signals:
    //void viewUpdated();

private:
    // Camera Attributes
    QVector3D Position=QVector3D(0.0f,0.0f,3.0f);
    QVector3D Front=QVector3D(0.0f,0.0f,-1.0f);
    QVector3D Up=QVector3D(0.0f,1.0f,0.0f);
    QVector3D Right;
    QVector3D WorldUp=Up;
    // Euler Angles
    float Yaw=YAW;
    float Pitch=PITCH;
    // Camera options
    float MovementSpeed=SPEED;
    float MouseSensitivity=SENSITIVITY;
    float Zoom=ZOOM;

    //
    QPoint MousePos;
};

#endif // MYCAMERA_H
