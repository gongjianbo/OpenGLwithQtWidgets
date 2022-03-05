#include "GLFrameBuffer.h"

GLFrameBuffer::GLFrameBuffer(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLFrameBuffer::~GLFrameBuffer()
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
    glDeleteFramebuffers(1, &framebuffer);
    doneCurrent();
}

void GLFrameBuffer::initializeGL()
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
        0.5f,   0.5f, 0.0f,   1.0f, 1.0f, // top right
        0.5f,  -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // top left
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

    //为了简化操作，我这里将帧缓冲固定为400x400大小
    int SCR_WIDTH = 400;
    int SCR_HEIGHT = 400;
    //创建帧缓冲对象
    //unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    //绑定为激活的帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    //添加一个纹理附件
    //unsigned int texturebuffer;
    glGenTextures(1, &texturebuffer);
    glBindTexture(GL_TEXTURE_2D, texturebuffer);
    //只设置了大小，分配了内存但是没填充数据（NULL），填充这个纹理将会在我们渲染到帧缓冲之后来进行
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //纹理附加到帧缓冲
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturebuffer, 0);
    //添加一个渲染缓冲附件
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    //完成所有操作后，检测帧缓冲是否完整
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
    }
    //切换到OpenGL默认帧缓冲
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //释放帧缓冲对象
    //glDeleteFramebuffers(1, &framebuffer);
}

void GLFrameBuffer::paintGL()
{
    //渲染自定义帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
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

    //切换到QOpenGLWidget.context默认的帧缓冲，这里不能用0来作为默认帧缓冲
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
    glBindTexture(GL_TEXTURE_2D, texturebuffer);	// use the color attachment texture as the texture of the quad plane
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    shaderProgram.release();
}

void GLFrameBuffer::resizeGL(int width, int height)
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
