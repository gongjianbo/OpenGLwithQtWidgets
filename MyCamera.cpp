#include "MyCamera.h"

#include <QDebug>

MyCamera::MyCamera(QObject *parent)
    :QObject(parent)
{
    updateCameraVectors();
}

MyCamera::MyCamera(const QVector3D &position,
                   const QVector3D &up,
                   float yaw, float pitch, QObject *parent)
    :QObject(parent),
      Position(position),
      Up(up),
      Yaw(yaw),
      Pitch(pitch)
{
    updateCameraVectors();
}

MyCamera::~MyCamera()
{

}

QMatrix4x4 MyCamera::getViewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(Position,Position+Front,Up);
    return view;
}

QVector3D MyCamera::getPosition() const
{
    return Position;
}

float MyCamera::getZoom() const
{
    return Zoom;
}

void MyCamera::keyPress(int key)
{
    //教程中会根据时间差来计算速度，这样在每个系统移动速度都一致
    //但是我这里先忽略掉这个需求
    float velocity = MovementSpeed*0.1;//deltaTime;
    switch (key) {
    case Qt::Key_W: //forward
    case Qt::Key_Up:
        Position+=Front * velocity;
        break;
    case Qt::Key_S://backward
    case Qt::Key_Down:
        Position-=Front * velocity;
        break;
    case Qt::Key_A: //left
    case Qt::Key_Left:
        Position-=Right * velocity;
        break;
    case Qt::Key_D: //right
    case Qt::Key_Right:
        Position+=Right * velocity;
        break;
    default:
        break;
    }
}

void MyCamera::mousePress(QPoint pos)
{
    MousePos=pos;
}

void MyCamera::mouseMove(QPoint pos)
{
    int x_offset=pos.x()-MousePos.x();
    int y_offset=pos.y()-MousePos.y();
    MousePos=pos;
    Yaw -= x_offset*MouseSensitivity;
    Pitch += y_offset*MouseSensitivity;

    if (Pitch > 89.0f)
        Pitch = 89.0f;
    else if (Pitch < -89.0f)
        Pitch = -89.0f;
    updateCameraVectors();
}

void MyCamera::mouseRelease(QPoint pos)
{
    Q_UNUSED(pos)
}

void MyCamera::mouseWheel(int delta)
{
    //缩放限制
    if(delta<0){
        Zoom+=MovementSpeed;
        if(Zoom>90)
            Zoom=90;
    }else{
        Zoom-=MovementSpeed;
        if(Zoom<1)
            Zoom=1;
    }
}

void MyCamera::updateCameraVectors()
{
    // Calculate the new Front vector
    QVector3D front;
    front.setX(cos(Yaw) * cos(Pitch));
    front.setY(sin(Pitch));
    front.setZ(sin(Yaw) * cos(Pitch));
    Front = front.normalized();
    Right = QVector3D::crossProduct(Front, WorldUp).normalized();
    Up = QVector3D::crossProduct(Right, Front).normalized();
}
