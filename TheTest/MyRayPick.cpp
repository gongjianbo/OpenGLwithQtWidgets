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

static const char *box_vertex=R"(#version 450 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
out vec3 theColor;
uniform mat4 mvp;
void main()
{
gl_Position = mvp * vec4(inPos, 1.0);
theColor = inColor;
})";

static const char *box_fragment=R"(#version 450 core
in vec3 theColor;
out vec4 fragColor;
void main()
{
fragColor = vec4(theColor, 1.0);
})";

//顶点数据（盒子六个面，一个面两个三角）
static float box_vertices[] = {
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    0.5f,  -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f
};


MyRayPick::MyRayPick(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus); //默认没有焦点

    rotationQuat = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 30.0f);
    rotationQuat *= QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -10.0f);
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
    boxVbo.destroy();
    boxVao.destroy();
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

    //【】盒子
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!boxProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,box_vertex)){
        qDebug()<<"compiler vertex error"<<boxProgram.log();
    }
    if(!boxProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,box_fragment)){
        qDebug()<<"compiler fragment error"<<boxProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!boxProgram.link()){
        qDebug()<<"link shaderprogram error"<<boxProgram.log();
    }

    boxVao.create();
    boxVao.bind();
    boxVbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    boxVbo.create();
    boxVbo.bind();
    boxVbo.allocate(box_vertices,sizeof(box_vertices));

    //参数1对应layout
    // position attribute
    boxProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    boxProgram.enableAttributeArray(0);
    // color attribute
    boxProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    boxProgram.enableAttributeArray(1);
}

void MyRayPick::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //因为我们使用了深度测试，需要在每次渲染迭代之前清除深度缓冲
    //（否则前一帧的深度信息仍然保存在缓冲中）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer)，也被称为深度缓冲(Depth Buffer)
    //（不开启深度缓冲的话，盒子的纹理堆叠顺序就是乱的）
    glEnable(GL_DEPTH_TEST); //默认关闭的

    //观察矩阵
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -5.0f);
    view.rotate(rotationQuat);
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    //模型矩阵
    QMatrix4x4 model;

    boxProgram.bind();
    boxVao.bind();
    boxProgram.setUniformValue("mvp", projection * view * model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    boxVao.release();
    boxProgram.release();

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

    makeCurrent();
    //rayData.append(QVector3D(0,0,0.8f));
    //rayData.append(QVector3D(0,0,-0.8f));
    rayData.append(ray_p1);
    rayData.append(ray_p2);
    rayVao.bind();
    if(rayVbo.isCreated()){
        rayVbo.destroy();
    }
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
