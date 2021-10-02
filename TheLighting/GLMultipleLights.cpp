#include "GLMultipleLights.h"
#include <cmath>
#include <QtMath>
#include <QDebug>

GLMultipleLights::GLMultipleLights(QWidget *parent)
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

GLMultipleLights::~GLMultipleLights()
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

void GLMultipleLights::initializeGL()
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

//多个光源
static QVector3D pointLightPositions[] = {
    QVector3D( 0.0f,  2.0f,  0.0f),
    QVector3D( 2.3f, -3.0f, -1.0f),
    QVector3D(-3.0f,  2.0f, 0.5f),
    QVector3D( 2.0f,  3.0f, 2.0f)
};

void GLMultipleLights::paintGL()
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
    // Directional light
    lightingShader.setUniformValue("dirLight.direction", -0.2f, -1.0f, -0.3f);
    lightingShader.setUniformValue("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setUniformValue("dirLight.diffuse", 0.2f, 0.2f, 0.2f);
    lightingShader.setUniformValue("dirLight.specular", 0.5f, 0.5f, 0.5f);
    QVector3D ambient_color(0.05f, 0.05f, 0.05f);
    QVector3D diffuse_color(0.0f, 0.0f, 0.4f);
    QVector3D specular_color(0.0f, 0.0f, 1.0f);
    // Point light 1
    lightingShader.setUniformValue("pointLights[0].position", pointLightPositions[0]);
    lightingShader.setUniformValue("pointLights[0].ambient", ambient_color);
    lightingShader.setUniformValue("pointLights[0].diffuse", diffuse_color);
    lightingShader.setUniformValue("pointLights[0].specular", specular_color);
    lightingShader.setUniformValue("pointLights[0].constant", 1.0f);
    lightingShader.setUniformValue("pointLights[0].linear", 0.09f);
    lightingShader.setUniformValue("pointLights[0].quadratic", 0.032f);
    // Point light 2
    lightingShader.setUniformValue("pointLights[1].position", pointLightPositions[1]);
    lightingShader.setUniformValue("pointLights[1].ambient", ambient_color);
    lightingShader.setUniformValue("pointLights[1].diffuse", diffuse_color);
    lightingShader.setUniformValue("pointLights[1].specular", specular_color);
    lightingShader.setUniformValue("pointLights[1].constant", 1.0f);
    lightingShader.setUniformValue("pointLights[1].linear", 0.09f);
    lightingShader.setUniformValue("pointLights[1].quadratic", 0.032f);
    // Point light 3
    lightingShader.setUniformValue("pointLights[2].position", pointLightPositions[2]);
    lightingShader.setUniformValue("pointLights[2].ambient", ambient_color);
    lightingShader.setUniformValue("pointLights[2].diffuse", diffuse_color);
    lightingShader.setUniformValue("pointLights[2].specular", specular_color);
    lightingShader.setUniformValue("pointLights[2].constant", 1.0f);
    lightingShader.setUniformValue("pointLights[2].linear", 0.09f);
    lightingShader.setUniformValue("pointLights[2].quadratic", 0.032f);
    // Point light 4
    lightingShader.setUniformValue("pointLights[3].position", pointLightPositions[3]);
    lightingShader.setUniformValue("pointLights[3].ambient", ambient_color);
    lightingShader.setUniformValue("pointLights[3].diffuse", diffuse_color);
    lightingShader.setUniformValue("pointLights[3].specular", specular_color);
    lightingShader.setUniformValue("pointLights[3].constant", 1.0f);
    lightingShader.setUniformValue("pointLights[3].linear", 0.09f);
    lightingShader.setUniformValue("pointLights[3].quadratic", 0.032f);
    // SpotLight
    QMatrix4x4 model;//模型矩阵
    model.translate(pointLightPositions[0]);
    model.scale(0.2f);
    QVector3D light_pos = model.map(QVector3D(0.0f, 0.0f, 0.0f));
    QVector3D direction_pos = QVector3D(0.0f, -20.0f, 0.0f);
    lightingShader.setUniformValue("spotLight.direction", direction_pos);
    lightingShader.setUniformValue("spotLight.position", light_pos);
    lightingShader.setUniformValue("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    lightingShader.setUniformValue("spotLight.diffuse", 0.3f, 1.0f, 0.3f);
    lightingShader.setUniformValue("spotLight.specular", 0.3f, 1.0f, 0.3f);
    lightingShader.setUniformValue("spotLight.constant", 1.0f);
    lightingShader.setUniformValue("spotLight.linear", 0.09f);
    lightingShader.setUniformValue("spotLight.quadratic", 0.032f);
    lightingShader.setUniformValue("spotLight.cutOff", (float)std::cos(qDegreesToRadians(12.5)));
    lightingShader.setUniformValue("spotLight.outerCutOff", (float)std::cos(qDegreesToRadians(15.0)));
    //材质-material properties
    //shininess影响镜面高光的散射/半径
    lightingShader.setUniformValue("material.shininess", 32.0f);
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
    lampVao.bind();
    for (unsigned int i = 0; i < 4; i++) {
        //模型矩阵
        QMatrix4x4 lamb_model;
        lamb_model.translate(pointLightPositions[i]);
        lamb_model.scale(0.2f);
        //传入着色器并绘制
        lampShader.setUniformValue("model", lamb_model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    lampVao.release();
    lampShader.release();
}

void GLMultipleLights::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLMultipleLights::initShader()
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
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // Properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // == ======================================
    // Our lighting is set up in 3 phases: directional, point lights and an optional flashlight
    // For each phase, a calculate function is defined that calculates the corresponding color
    // per lamp. In the main() function we take all the calculated colors and sum them up for
    // this fragment's final color.
    // == ======================================
    // Phase 1: Directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    // Phase 2: Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    // Phase 3: Spot light
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    color = vec4(result, 1.0);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // Combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
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

QOpenGLTexture *GLMultipleLights::initTexture(const QString &imgpath)
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
