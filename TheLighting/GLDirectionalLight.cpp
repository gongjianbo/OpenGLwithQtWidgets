#include "GLDirectionalLight.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

GLDirectionalLight::GLDirectionalLight(QWidget *parent)
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

GLDirectionalLight::~GLDirectionalLight()
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
    delete diffuseMap;
    delete specularMap;
    doneCurrent();
}

void GLDirectionalLight::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();
    initShader();

    //方块的顶点、法向量、纹理坐标
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();

    //light vao
    lightingVao.create();
    lightingVao.bind();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    lightingShader.setAttributeBuffer(0, GL_FLOAT, sizeof(GLfloat) * 0, 3, sizeof(GLfloat) * 8);
    lightingShader.enableAttributeArray(0);
    lightingShader.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8);
    lightingShader.enableAttributeArray(1);
    lightingShader.setAttributeBuffer(2, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8);
    lightingShader.enableAttributeArray(2);
    vbo.release();
    lightingVao.release();

    //lamp vao
    lampVao.create();
    lampVao.bind();
    vbo.bind();
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    lampShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    lampShader.enableAttributeArray(0);
    vbo.release();
    lampVao.release();

    //纹理
    diffuseMap = initTexture(":/container2.png");
    specularMap = initTexture(":/container2_specular.png");
    //shader configuration
    lightingShader.bind();
    lightingShader.setUniformValue("material.diffuse", 0);
    lightingShader.setUniformValue("material.specular", 1);
    lightingShader.release();

    //timer.start();
}

//绘制多个盒子
static QVector3D cubePositions[] = {
    QVector3D( 0.0f,  0.0f,  0.0f),
    QVector3D( 0.0f, -4.0f,  0.0f),
    QVector3D( 0.0f,  4.0f,  0.0f),
    QVector3D( 1.0f, -5.0f, 1.0f),
    QVector3D(-1.5f, -2.2f, -2.5f),
    QVector3D(-3.8f, -2.0f, -7.3f),
    QVector3D( 2.4f, -0.4f, -3.5f),
    QVector3D(-1.7f, -3.0f, -6.5f),
    QVector3D( 5.3f, -2.0f, -2.5f),
    QVector3D(-1.3f,  1.0f, -1.5f)
};

void GLDirectionalLight::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    //draw lighting
    lightingShader.bind();
    QMatrix4x4 view; //观察矩阵
    view.translate(0.0f, 0.0f, -10.0f);
    view.rotate(45, QVector3D(1.0f, 0.8f, 0.0f));
    lightingShader.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    lightingShader.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    //lightingShader.setUniformValue("model", model);
    //因为要获取灯的位置，所以提前算灯的model矩阵
    model = QMatrix4x4();
    model.translate(QVector3D(0.0f, 2.0f, 0.0f));
    model.scale(0.2f);
    //QVector3D light_pos = model.map(QVector3D(0.0f, 0.0f, 0.0f));
    //y上方向下照射的光
    QVector3D direction_pos = QVector3D(0.0f, -10.0f, 0.0f);
    QMatrix4x4 vv = view.inverted(); //逆矩阵求观察点位置
    QVector3D view_pos = vv.map(QVector3D(0.0f, 0.0f, 0.0f));
    lightingShader.setUniformValue("light.direction", direction_pos);
    lightingShader.setUniformValue("viewPos", view_pos);
    //光照-light properties
    QVector3D light_color = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D diffuse_color = light_color * 0.5f; // decrease the influence
    QVector3D ambient_color = diffuse_color * 0.2f; // low influence
    lightingShader.setUniformValue("light.ambient", ambient_color);
    lightingShader.setUniformValue("light.diffuse", diffuse_color);
    lightingShader.setUniformValue("light.specular", QVector3D(1.0f, 1.0f, 1.0f));

    //材质-material properties
    //shininess影响镜面高光的散射/半径
    lightingShader.setUniformValue("material.shininess", 64.0f);
    lightingVao.bind();
    //绑定2d纹理
    //bind diffuse map
    glActiveTexture(GL_TEXTURE0);
    diffuseMap->bind();
    //bind specular map
    glActiveTexture(GL_TEXTURE1);
    specularMap->bind();
    //glDrawArrays(GL_TRIANGLES, 0, 36);
    //多个盒子便于对比
    for (unsigned int i = 0; i < 10; i++) {
        //模型矩阵
        QMatrix4x4 box_model;
        //平移
        box_model.translate(cubePositions[i]);
        float angle = 20.0f * i;
        //旋转
        box_model.rotate(angle, QVector3D(1.0f, 0.3f, 0.5f));
        //传入着色器并绘制
        lightingShader.setUniformValue("model", box_model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
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

void GLDirectionalLight::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLDirectionalLight::initShader()
{
    //lingting shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *lighting_vertex=R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
})";
    const char *lighting_fragment=R"(#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

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

QOpenGLTexture *GLDirectionalLight::initTexture(const QString &imgpath)
{
    QOpenGLTexture *texture = new QOpenGLTexture(QImage(imgpath), QOpenGLTexture::GenerateMipMaps);
    if(!texture->isCreated()){
        qDebug() << "Failed to create texture";
    }
    //set the texture wrapping parameters
    //等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    //set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    return texture;
}
