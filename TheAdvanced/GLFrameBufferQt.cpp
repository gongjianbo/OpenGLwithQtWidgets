#include "GLFrameBufferQt.h"

GLFrameBufferQt::GLFrameBufferQt(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLFrameBufferQt::~GLFrameBufferQt()
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
    delete fbo;
    delete texture;
    doneCurrent();
}

void GLFrameBufferQt::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str = R"(#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;
uniform mat4 mvp;
out vec2 texCoord;
void main() {
gl_Position = mvp * vec4(inPos, 1.0);
texCoord = vec2(inTexCoord.x, inTexCoord.y);
})";
    const char *fragment_str = R"(#version 450
uniform sampler2D theTexture;
in vec2 texCoord;
out vec4 fragColor;
void main() {
fragColor = texture(theTexture, texCoord);
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
        // positions          // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // top right
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f  // top left
    };
    //索引
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    vao.create();
    vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));

    ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.allocate(indices,sizeof(indices));

    // position attribute
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    shaderProgram.enableAttributeArray(0);
    // texture coord attribute
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    shaderProgram.enableAttributeArray(1);

    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture = new QOpenGLTexture(QImage(":/awesomeface.png").mirrored(), QOpenGLTexture::GenerateMipMaps);
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

    //为了简化操作，暂时将帧缓冲固定为400x400大小
    fbo = new QOpenGLFramebufferObject(400, 400, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D, GL_RGBA);
    fbo->bind();
    fbo->addColorAttachment(fbo->size(), GL_RGBA);
    fbo->release();
}

void GLFrameBufferQt::paintGL()
{
    //渲染自定义帧缓冲
    fbo->bind();
    glViewport(0, 0, 400, 400);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
    shaderProgram.setUniformValue("mvp", QMatrix4x4());
    //绘制
    vao.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    shaderProgram.release();

    //切换到QOpenGLWidget默认的帧缓冲，这里不能用0来作为默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, width(), height());
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProgram.bind();
    //texture->bind();
    QMatrix4x4 view2;
    view2.translate(QVector3D(0.0f, 0.0f, -10.0f));
    shaderProgram.setUniformValue("mvp", projection * view2);
    vao.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    shaderProgram.release();
}

void GLFrameBufferQt::resizeGL(int width, int height)
{
    if (width < 1 || height < 1) {
        return;
    }
    //宽高比例
    qreal aspect = qreal(width) / qreal(height);
    //单位矩阵
    projection.setToIdentity();
    //坐标到达观察空间之后，我们需要将其投影到裁剪坐标。
    //裁剪坐标会被处理至-1.0到1.0的范围内，并判断哪些顶点将会出现在屏幕上
    //float left, float right, float bottom, float top, float nearPlane, float farPlane
    projection.ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, 0.0f, 100.0f);
}
