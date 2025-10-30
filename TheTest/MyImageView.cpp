#include "MyImageView.h"

//着色器代码
#if 1
const char *vertex_str=R"(#version 330 core
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inTexCoord;
uniform mat4 projection;
out vec2 texCoord;
void main()
{
gl_Position = projection * vec4(inPos, 0.0, 1.0);
texCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
})";
const char *fragment_str=R"(#version 330 core
uniform sampler2D theTexture;
in vec2 texCoord;
out vec4 fragColor;
void main()
{
fragColor = texture(theTexture, texCoord);
})";
#else
//兼容opengl es2
const char *vertex_str = R"(
#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif
attribute vec2 inPos;
attribute vec2 inTexCoord;
uniform mat4 projection;
varying vec2 texCoord;
void main() {
gl_Position = projection * vec4(inPos, 0.0, 1.0);
texCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
})";
const char *fragment_str = R"(
#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif
uniform sampler2D theTexture;
varying vec2 texCoord;
void main() {
gl_FragColor = texture2D(theTexture, texCoord);
})";
#endif

MyImageView::MyImageView(QWidget *parent)
    : QOpenGLWidget{parent}
{
    setImage(QImage(":/fishpond.jpg"));
}

MyImageView::~MyImageView()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if (!isValid())
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

void MyImageView::setImage(const QImage &image)
{
    cache = image;
    // cache = image.convertToFormat(QImage::QImage::Format_ARGB32);
    if (!isValid()) {
        return;
    }

    makeCurrent();
    updateTexture();
    doneCurrent();

    updateProjection(width(), height(), image.width(), image.height());
    update();
}

void MyImageView::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if (!shaderProgram.addCacheableShaderFromSourceCode(
            QOpenGLShader::Vertex, vertex_str)){
        qDebug() << "compiler vertex error" << shaderProgram.log();
    }
    if (!shaderProgram.addCacheableShaderFromSourceCode(
            QOpenGLShader::Fragment, fragment_str)){
        qDebug() << "compiler fragment error" << shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if (!shaderProgram.link()) {
        qDebug() << "link shaderprogram error" << shaderProgram.log();
    }

    //矩形顶点
    float vertices[] = {
        // positions          // texture coords
        1.0f,  1.0f,   1.0f, 1.0f, // top right
        1.0f, -1.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        -1.0f,  1.0f,  0.0f, 1.0f  // top left
    };
    //索引
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    vao.create();
    vao.bind();

    vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));

    // position attribute
    int attr = -1;
    attr = shaderProgram.attributeLocation("inPos");
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 2, sizeof(GLfloat) * 4);
    shaderProgram.enableAttributeArray(attr);
    // texture coord attribute
    attr = shaderProgram.attributeLocation("inTexCoord");
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 2, 2, sizeof(GLfloat) * 4);
    shaderProgram.enableAttributeArray(attr);

    shaderProgram.bind();
    shaderProgram.setUniformValue("theTexture", 0);
    shaderProgram.release();

    updateTexture();
}

void MyImageView::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!texture->isCreated()) return;

    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    shaderProgram.bind();
    vao.bind();

    shaderProgram.setUniformValue("projection", projection);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    vao.release();
    texture->release();
    shaderProgram.release();
}

void MyImageView::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    if (!texture->isCreated()) return;
    updateProjection(width, height, texture->width(), texture->height());
}

void MyImageView::updateTexture()
{
    if (cache.isNull()) return;

    if (texture) delete texture;
    // texture
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture = new QOpenGLTexture(cache, QOpenGLTexture::GenerateMipMaps);
    if (!texture->isCreated()) {
        qDebug() << "Failed to load texture";
        return;
    }

    // set the texture wrapping parameters
    //等于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //ClampToEdge图片边缘看起来更正常一点
    texture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    texture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // set texture filtering parameters
    //等于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    //texture中已经持有一份，可以把缓存的QImage清空了
    cache = QImage();
}

void MyImageView::updateProjection(int viewW, int viewH, int imageW, int imageH)
{
    if (viewW < 1 || viewH < 1 || imageW < 1 || imageH < 1)
        return;

    projection.setToIdentity();

    float viewRatio = float(viewW) / float(viewH);
    float imageRatio = float(imageW) / float(imageH);
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    //imageRatio比viewRatio小则图片宽度偏小，两边要加padding
    //imageRatio比viewRatio大则图片高度偏小，上下要加padding
    if (viewRatio > imageRatio) {
        scaleX = viewRatio / imageRatio;
    } else {
        scaleY = imageRatio / viewRatio;
    }
    //计算正交投影，使图像保持原比例
    projection.ortho(-scaleX, scaleX, -scaleY, scaleY, -1.0f, 1.0f);
}
