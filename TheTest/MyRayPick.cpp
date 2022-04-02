#include "MyRayPick.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

static const char *ray_vertex=R"(#version 450 core
layout (location = 0) in vec3 inPos;
uniform mat4 mvp;
void main()
{
gl_Position = mvp * vec4(inPos, 1.0);
})";

static const char *ray_fragment=R"(#version 450 core
out vec4 fragColor;
void main()
{
fragColor = vec4(0.0, 0.8, 0.0, 1.0);
})";

//note这里两个vec直接比较不知道会不会有问题
static const char *ele_vertex=R"(#version 450 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
out vec3 theColor;
uniform mat4 mvp;
uniform vec3 selectColor;
void main()
{
gl_Position = mvp * vec4(inPos, 1.0);
if(selectColor==inColor){
theColor = vec3(1.0, 1.0, 1.0);
}else{
theColor = inColor;
}
})";

static const char *ele_fragment=R"(#version 450 core
in vec3 theColor;
out vec4 fragColor;
void main()
{
fragColor = vec4(theColor, 1.0);
})";

MyRayPick::MyRayPick(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus); //默认没有焦点

    //rotationQuat = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 30.0f);
    //rotationQuat *= QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -10.0f);
}

MyRayPick::~MyRayPick()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    if(rayVbo.isCreated()){
        rayVbo.destroy();
    }
    rayVao.destroy();
    eleVbo.destroy();
    eleVao.destroy();
    doneCurrent();
}

void MyRayPick::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //【】射线
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!rayProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,ray_vertex)){
        qDebug()<<"compiler vertex error"<<rayProgram.log();
    }
    if(!rayProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,ray_fragment)){
        qDebug()<<"compiler fragment error"<<rayProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!rayProgram.link()){
        qDebug()<<"link shaderprogram error"<<rayProgram.log();
    }
    rayVao.create();
    rayVao.bind();
    rayVbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    //【】图元
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!eleProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,ele_vertex)){
        qDebug()<<"compiler vertex error"<<eleProgram.log();
    }
    if(!eleProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,ele_fragment)){
        qDebug()<<"compiler fragment error"<<eleProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!eleProgram.link()){
        qDebug()<<"link shaderprogram error"<<eleProgram.log();
    }

    eleVertex = QVector<QVector3D>{
            QVector3D(1.0f, 0.0f, -0.5f),  QVector3D(1.0f, 0.0f, 0.0f),
            QVector3D(-0.5f, -0.5f, -0.5f),  QVector3D(1.0f, 0.0f, 0.0f),
            QVector3D(-0.5f, 0.5f, -0.5f),  QVector3D(1.0f, 0.0f, 0.0f),

            QVector3D(-1.0f, 0.0f, -0.1f),  QVector3D(0.0f, 1.0f, 0.0f),
            QVector3D(0.0f, 0.5f, -0.1f),  QVector3D(0.0f, 1.0f, 0.0f),
            QVector3D(0.0f, -0.5f, -0.1f),  QVector3D(0.0f, 1.0f, 0.0f),

            QVector3D(0.6f, 0.0f, 0.1f),  QVector3D(0.0f, 0.0f, 1.0f),
            QVector3D(1.0f, 0.0f, -0.3f),  QVector3D(0.0f, 0.0f, 1.0f),
            QVector3D(1.0f, 0.5f, -0.5f),  QVector3D(0.0f, 0.0f, 1.0f)};

    eleVao.create();
    eleVao.bind();
    eleVbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    eleVbo.create();
    eleVbo.bind();
    eleVbo.allocate((void*)eleVertex.data(),sizeof(QVector3D)*eleVertex.size());

    //参数1对应layout
    // position attribute
    eleProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    eleProgram.enableAttributeArray(0);
    // color attribute
    eleProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    eleProgram.enableAttributeArray(1);

    //深度缓冲
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void MyRayPick::paintGL()
{
    //清除缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //观察矩阵
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -5.0f);
    view.rotate(rotationQuat);
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    //模型矩阵
    QMatrix4x4 model;

    eleProgram.bind();
    eleVao.bind();
    eleProgram.setUniformValue("mvp", projection * view * model);
    eleProgram.setUniformValue("selectColor", selectColor);
    glDrawArrays(GL_TRIANGLES, 0, eleVertex.size()/2);
    eleVao.release();
    eleProgram.release();

    if(rayData.size()>0)
    {
        rayProgram.bind();
        rayVao.bind();
        rayProgram.setUniformValue("mvp", projection * view * model);
        glDrawArrays(GL_LINES, 0, rayData.size());
        rayVao.release();
        rayProgram.release();
    }
}

void MyRayPick::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void MyRayPick::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
    //右键点击添加射线
    if(event->button()==Qt::RightButton){
        appendRay(mousePos);
    }
}

void MyRayPick::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void MyRayPick::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    //参照示例cube
    QVector2D diff = QVector2D(event->pos()) - QVector2D(mousePos);
    mousePos = event->pos();
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    rotationAxis = (rotationAxis + n).normalized();
    //不能对换乘的顺序
    rotationQuat = QQuaternion::fromAxisAndAngle(rotationAxis, 1.0f) * rotationQuat;

    update();
}

void MyRayPick::wheelEvent(QWheelEvent *event)
{
    event->accept();
    //fovy越小，模型看起来越大
    if(event->delta() < 0){
        //鼠标向下滑动为-，这里作为zoom out
        projectionFovy += 0.5f;
        if(projectionFovy > 90)
            projectionFovy = 90;
    }else{
        //鼠标向上滑动为+，这里作为zoom in
        projectionFovy -= 0.5f;
        if(projectionFovy < 1)
            projectionFovy = 1;
    }
    update();
}

void MyRayPick::appendRay(const QPointF &pos)
{
    //视口坐标转标准化设备(NDC)坐标
    float ndc_x = 2.0f * pos.x() / width() - 1.0f;
    float ndc_y = 1.0f - (2.0f * pos.y() / height());
    QVector3D ray_p1 = calcVec3(ndc_x, ndc_y, 1.0f);
    QVector3D ray_p2 = calcVec3(ndc_x, ndc_y, -1.0f);
    //判断射线与三角碰撞情况，然后选离观察点最近的三角
    int check_index = -1;
    float check_value = -1;
    for(int i=0;i<eleVertex.size();i+=2*3)
    {
        float check = checkTriCollision(ray_p2, ray_p1,
                                        eleVertex.at(i),
                                        eleVertex.at(i+2),
                                        eleVertex.at(i+4));
        if(check > 0.0f){
            if(check_value <= 0.0f || check_value > check){
                check_index = i;
                check_value = check;
            }
        }
        qDebug()<<"check"<<i<<check;
    }
    if(check_index != -1){
        selectColor = eleVertex.at(check_index+1);
    }

    makeCurrent();
    //rayData.append(QVector3D(0,0,0.8f));
    //rayData.append(QVector3D(0,0,-0.8f));
    rayData.append(ray_p1);
    rayData.append(ray_p2);
    if(rayVbo.isCreated()){
        rayVbo.destroy();
    }
    rayVao.bind();
    rayVbo.create();
    rayVbo.bind();
    rayVbo.allocate(rayData.data(),rayData.size()*sizeof(QVector3D));
    rayProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    rayProgram.enableAttributeArray(0);
    doneCurrent();

    update();
}

QVector3D MyRayPick::calcVec3(float x, float y, float z)
{
    //观察矩阵
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -5.0f);
    view.rotate(rotationQuat);
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    //模型矩阵
    //QMatrix4x4 model;

    QVector3D ndc_ray = QVector3D(x, y, z);
    QVector4D clip_ray = QVector4D(ndc_ray, 1.0f);
    QVector4D eye_ray = projection.inverted() * clip_ray;
    QVector4D world_ray = view.inverted() * eye_ray;
    //转笛卡尔坐标
    if(world_ray.w() != 0.0f){
        world_ray /= world_ray.w();
    }

    return world_ray.toVector3D();
}

float MyRayPick::checkTriCollision(QVector3D pos, QVector3D ray,
                                   QVector3D vtx1, QVector3D vtx2, QVector3D vtx3)
{
    QVector3D E1 = vtx2 - vtx1;
    QVector3D E2 = vtx3 - vtx1;
    QVector3D P = QVector3D::crossProduct(ray, E2);
    float det = QVector3D::dotProduct(P, E1);
    QVector3D T;
    if(det > 0)
        T = pos - vtx1;
    else{
        T = vtx1 - pos;
        det = -det;
    }

    //表示射线与三角面所在的平面平行，返回不相交
    if(det < 0.00001f)
        return -1.0f;

    //相交则判断 交点是否落在三角形面内
    float u = QVector3D::dotProduct(P, T);
    if(u < 0.0f || u > det)
        return -1.0f;

    QVector3D Q = QVector3D::crossProduct(T, E1);
    float v = QVector3D::dotProduct(Q, ray);
    if(v < 0.0f || u+v > det)
        return -1.0f;

    float t = QVector3D::dotProduct(Q, E2);
    if(t < 0.0f)
        return -1.0f;
    return t/det;
}
