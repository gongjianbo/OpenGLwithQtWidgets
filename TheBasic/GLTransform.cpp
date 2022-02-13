#include "GLTransform.h"
#include <QMatrix4x4>
#include <QDebug>

GLTransform::GLTransform(QWidget *parent)
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

GLTransform::~GLTransform()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    vbo.destroy();
    ebo.destroy();
    vao.destroy();
    delete texture1;
    delete texture2;
    doneCurrent();
}

void GLTransform::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //因为OpenGL纹理颠倒过来的，所以取反vec2(aTexCoord.x, 1-aTexCoord.y);
    const char *vertex_str=R"(#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
uniform mat4 transform;
out vec3 theColor;
out vec2 texCoord;
void main()
{
gl_Position = transform*vec4(inPos, 1.0);
theColor = inColor;
texCoord = vec2(inTexCoord.x, 1-inTexCoord.y);
})";
    const char *fragment_str=R"(#version 330 core
in vec3 theColor;
in vec2 texCoord;
uniform sampler2D texture1;
uniform sampler2D texture2;
out vec4 fragColor;
void main()
{
fragColor = mix(texture(texture1, texCoord),
texture(texture2, texCoord), 0.2) * vec4(theColor, 1.0);
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<shaderProgram.log();
    }
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<shaderProgram.log();
    }

    //顶点数据
    float vertices[] = {
        // positions          // colors           // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };
    //索引
    unsigned int indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    vao.create();
    vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));

    ebo=QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.allocate(indices,sizeof(indices));

    //参数1对应layout
    // position attribute
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(0);
    // color attribute
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(1);
    // texture coord attribute
    shaderProgram.setAttributeBuffer(2, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(2);

    // texture 1
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture1 = new QOpenGLTexture(QImage(":/container.jpg"), QOpenGLTexture::GenerateMipMaps);
    if(!texture1->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture1->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture1->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    // set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture1->setMinificationFilter(QOpenGLTexture::Linear);
    texture1->setMagnificationFilter(QOpenGLTexture::Linear);

    // texture 2
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture2 = new QOpenGLTexture(QImage(":/awesomeface.png"), QOpenGLTexture::GenerateMipMaps);
    if(!texture2->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture2->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture2->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    // set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture2->setMinificationFilter(QOpenGLTexture::Linear);
    texture2->setMagnificationFilter(QOpenGLTexture::Linear);

    shaderProgram.bind();
    shaderProgram.setUniformValue("texture1", 0);
    shaderProgram.setUniformValue("texture2", 1);
    shaderProgram.release();

    timer.start();
}

void GLTransform::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //纹理
    glActiveTexture(GL_TEXTURE0);
    texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    texture2->bind();

    //变换矩阵
    QMatrix4x4 transform;
    //向右下角平移
    transform.translate(QVector3D(0.5f, -0.5f, 0.0f));
    //绕z轴旋转
    transform.rotate(rotate, QVector3D(0.0f, 0.0f, 1.0f));

    shaderProgram.bind();
    shaderProgram.setUniformValue("transform", transform);
    vao.bind();
    //根据索引绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GLTransform::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
