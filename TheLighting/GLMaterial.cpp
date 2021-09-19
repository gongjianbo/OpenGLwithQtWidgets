#include "GLMaterial.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

GLMaterial::GLMaterial(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&timer,&QTimer::timeout,this,[this](){
        rotate+=1;
        if(isVisible()){
            update();
        }
    });
    timer.setInterval(50);
}

GLMaterial::~GLMaterial()
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

void GLMaterial::initializeGL()
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

void GLMaterial::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    //draw lighting
    lightingShader.bind();
    QMatrix4x4 view; //观察矩阵
    view.translate(0.0f, 0.0f, -5.0f);
    view.rotate(45, QVector3D(1.0f, 0.8f, 0.0f));
    lightingShader.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    lightingShader.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    lightingShader.setUniformValue("model", model);
    //因为要获取灯的位置，所以提前算灯的model矩阵
    model = QMatrix4x4();
    model.translate(QVector3D(1.0f, 1.0f, -1.0f));
    model.scale(0.3f);
    QVector3D light_pos = model.map(QVector3D(0.0f, 0.0f, 0.0f));
    QMatrix4x4 vv = view.inverted(); //逆矩阵求观察点位置
    QVector3D view_pos = vv.map(QVector3D(0.0f, 0.0f, 0.0f));
    lightingShader.setUniformValue("light.position", light_pos);
    lightingShader.setUniformValue("viewPos", view_pos);
    //光照-light properties
    QColor color = QColor::fromHsv((rotate*2)%360,255,255).toRgb(); //随时间变色
    QVector3D light_color = QVector3D(color.red()/256.0f, color.green()/256.0f, color.blue()/256.0f);
    QVector3D diffuse_color = light_color * 0.5f; // decrease the influence
    QVector3D ambient_color = diffuse_color * 0.2f; // low influence
    lightingShader.setUniformValue("light.ambient", ambient_color);
    lightingShader.setUniformValue("light.diffuse", diffuse_color);
    lightingShader.setUniformValue("light.specular", QVector3D(1.0f, 1.0f, 1.0f));

    //材质-material properties
    //ambient在环境光照下物体反射颜色
    lightingShader.setUniformValue("material.ambient", QVector3D(1.0f, 0.5f, 0.31f));
    //diffuse在漫反射光照下物体颜色
    lightingShader.setUniformValue("material.diffuse", QVector3D(1.0f, 0.5f, 0.31f));
    //specular镜面光照对物体的颜色影响(甚至可能反射一个物体特定的镜面高光颜色)
    //specular lighting doesn't have full effect on this object's material
    lightingShader.setUniformValue("material.specular", QVector3D(0.5f, 0.5f, 0.5f));
    //shininess影响镜面高光的散射/半径
    lightingShader.setUniformValue("material.shininess", 32.0f);

    lightingVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    lightingVao.release();
    lightingShader.release();

    //draw lamp
    lampShader.bind();
    lampShader.setUniformValue("view", view);
    lampShader.setUniformValue("projection", projection);
    lampShader.setUniformValue("model", model);
    lampVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    lampVao.release();
    lampShader.release();
}

void GLMaterial::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLMaterial::initShader()
{
    //lingting shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *lighting_vertex=R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * vec4(FragPos, 1.0);
})";
    const char *lighting_fragment=R"(#version 330 core
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = light.ambient * material.ambient;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
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
