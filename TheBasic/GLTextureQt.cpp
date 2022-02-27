#include "GLTextureQt.h"
#include <QDebug>

GLTextureQt::GLTextureQt(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTextureQt::~GLTextureQt()
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
    delete texture;
    doneCurrent();
}

void GLTextureQt::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部
    //--这里对纹理坐标的y进行的取反
    const char *vertex_str=R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
out vec3 theColor;
out vec2 texCoord;
void main() {
gl_Position = vec4(inPos,1.0);
theColor = inColor;
texCoord = vec2(inTexCoord.x, 1-inTexCoord.y);
})";
    const char *fragment_str=R"(#version 330 core
uniform sampler2D theTexture;
in vec3 theColor;
in vec2 texCoord;
out vec4 fragColor;
void main() {
fragColor = texture(theTexture, texCoord) * vec4(theColor, 1.0);
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
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };
    //索引
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
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

    // position attribute
    int attr = -1;
    attr = shaderProgram.attributeLocation("inPos");
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(attr);
    // color attribute
    attr = shaderProgram.attributeLocation("inColor");
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(attr);
    // texture coord attribute
    attr = shaderProgram.attributeLocation("inTexCoord");
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8);
    shaderProgram.enableAttributeArray(attr);

    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture = new QOpenGLTexture(QImage(":/awesomeface.png"), QOpenGLTexture::GenerateMipMaps);
    if(!texture->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    // set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    shaderProgram.bind();
    shaderProgram.setUniformValue("theTexture", 0);
    shaderProgram.release();
}

void GLTextureQt::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //深度测试发生于混合之前
    //当我们需要绘制透明图片时，就需要关闭GL_DEPTH_TEST并且打开混合glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //指定混合函数
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //激活纹理
    glActiveTexture(GL_TEXTURE0);
    texture->bind();

    shaderProgram.bind();
    //绘制
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vao.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    shaderProgram.release();
}

void GLTextureQt::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}
