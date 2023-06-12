#include "GLBasicLighting.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

GLBasicLighting::GLBasicLighting(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&timer,&QTimer::timeout,this,[this](){
        rotate+=2;
        if(isVisible()){
            update();
        }
    });
    timer.setInterval(50);
}

GLBasicLighting::~GLBasicLighting()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    vbo.destroy();
    lightingVao.destroy();
    lampVao.destroy();
    doneCurrent();
}

void GLBasicLighting::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();
    initShader();

    //方块的顶点和法向量
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();

    //light vao
    lightingVao.create();
    lightingVao.bind();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    lightingShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    lightingShader.enableAttributeArray(0);
    lightingShader.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    lightingShader.enableAttributeArray(1);
    vbo.release();
    lightingVao.release();

    //lamp vao
    lampVao.create();
    lampVao.bind();
    vbo.bind();
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    lampShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    lampShader.enableAttributeArray(0);
    vbo.release();
    lampVao.release();

    timer.start();
}

void GLBasicLighting::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    //draw lighting
    lightingShader.bind();
    lightingShader.setUniformValue("objectColor", QVector3D(1.0f,0.5f,0.31f));
    lightingShader.setUniformValue("lightColor", QVector3D(1.0f,1.0f,1.0f));
    QMatrix4x4 view; //观察矩阵
    float radius = 10.0f;
    view.translate(0.0f, 0.0f, -radius);
    view.rotate(rotationQuat);
    lightingShader.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    lightingShader.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    //model.rotate(30, QVector3D(1.0f, 1.0f, 0.0f)); //先不考虑旋转
    lightingShader.setUniformValue("model", model);
    //因为要获取灯的位置，所以提前算灯的model矩阵
    model = QMatrix4x4();
    float tx = std::sin(rotate*0.05) * 2.0f;
    float tz = std::cos(rotate*0.05) * 2.0f;
    model.translate(QVector3D(tx, 1.0f, tz));
    //model.rotate(30, QVector3D(1.0f, 1.0f, 0.0f));
    model.scale(0.3f);
    QVector3D light_pos = model.map(QVector3D(0.0f, 0.0f, 0.0f));
    QMatrix4x4 vv = view.inverted(); //逆矩阵求观察点位置
    QVector3D view_pos = vv.map(QVector3D(0.0f, 0.0f, 0.0f));
    lightingShader.setUniformValue("lightPos", light_pos);
    lightingShader.setUniformValue("viewPos", view_pos);
    lightingVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    lightingVao.release();
    lightingShader.release();

    //draw lamp
    lampShader.bind();
    lampShader.setUniformValue("view", view);
    lampShader.setUniformValue("projection", projection);
    //model = QMatrix4x4();
    //model.translate(QVector3D(1.0f, 1.0f, 0.0f));
    //model.rotate(30, QVector3D(1.0f, 1.0f, 0.0f));
    //model.scale(0.3f);
    lampShader.setUniformValue("model", model);
    lampVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    lampVao.release();
    lampShader.release();
}

void GLBasicLighting::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLBasicLighting::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
}

void GLBasicLighting::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void GLBasicLighting::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    //参照示例cube
    QVector2D diff = QVector2D(event->pos()) - QVector2D(mousePos);
    mousePos = event->pos();
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    rotationAxis = (rotationAxis + n).normalized();
    //不能对换乘的顺序
    rotationQuat = QQuaternion::fromAxisAndAngle(rotationAxis, 2.0f) * rotationQuat;

    update();
}

void GLBasicLighting::wheelEvent(QWheelEvent *event)
{
    event->accept();
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    //const QPoint pos = event->pos();
    const int delta = event->delta();
#else
    //const QPoint pos = event->position().toPoint();
    const int delta = event->angleDelta().y();
#endif
    //fovy越小，模型看起来越大
    if(delta < 0){
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

void GLBasicLighting::initShader()
{
    //lingting shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *lighting_vertex=R"(#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view *  model * vec4(position, 1.0f);
    FragPos = vec3(model * vec4(position, 1.0f));
    Normal = mat3(transpose(inverse(model))) * normal;
})";
    const char *lighting_fragment=R"(#version 330 core
in vec3 Normal;
in vec3 FragPos;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec4 FragColor;

void main()
{
// ambient
float ambientStrength = 0.1;
vec3 ambient = ambientStrength * lightColor;

// diffuse
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * lightColor;

// specular
float specularStrength = 0.5;
vec3 viewDir = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
vec3 specular = specularStrength * spec * lightColor;

vec3 result = (ambient + diffuse + specular) * objectColor;
FragColor = vec4(result, 1.0f);
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!lightingShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,lighting_vertex)){
        qDebug()<<"compiler vertex error"<<lightingShader.log();
    }
    if(!lightingShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,lighting_fragment)){
        qDebug()<<"compiler fragment error"<<lightingShader.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!lightingShader.link()){
        qDebug()<<"link shaderprogram error"<<lightingShader.log();
    }

    //lamp shader
    const char *lamp_vertex=R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
gl_Position = projection * view * model * vec4(aPos, 1.0f);
})";
    const char *lamp_fragment=R"(#version 330 core
out vec4 FragColor;
void main()
{
FragColor = vec4(1.0);
})"; // set alle 4 vector values to 1.0

    if(!lampShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,lamp_vertex)){
        qDebug()<<"compiler vertex error"<<lampShader.log();
    }
    if(!lampShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,lamp_fragment)){
        qDebug()<<"compiler fragment error"<<lampShader.log();
    }
    if(!lampShader.link()){
        qDebug()<<"link shaderprogram error"<<lampShader.log();
    }
}


