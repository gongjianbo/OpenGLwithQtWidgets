#include "MyTransform.h"

#include <QSurfaceFormat>
#include <QDebug>

MyTransform::MyTransform(QWidget *parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat fmt = format();
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(4, 5);
    //当启用多重采样时，将每个像素的首选样本数设置为numSamples
    fmt.setSamples(32);
    setFormat(fmt);
}

MyTransform::~MyTransform()
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
    doneCurrent();
}

void MyTransform::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str = R"(#version 450 core
layout (location = 0) in vec3 inPos;
uniform mat4 transform;
uniform vec3 color;
out vec3 theColor;
void main()
{
gl_Position = transform * vec4(inPos, 1.0);
theColor = color;
})";
    const char *fragment_str = R"(#version 450 core
in vec3 theColor;
out vec4 fragColor;
void main()
{
fragColor = vec4(theColor, 1.0);
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex, vertex_str)){
        qDebug()<<"compiler vertex error"<<shaderProgram.log();
    }
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment, fragment_str)){
        qDebug()<<"compiler fragment error"<<shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<shaderProgram.log();
    }

    //顶点数据
    GLfloat vertices[] = {
        // positions
        0.5f,  0.5f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    //索引
    GLuint indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    vao.create();
    vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    ebo=QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.allocate(indices, sizeof(indices));

    //参数对应layout
    // position attribute
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    shaderProgram.enableAttributeArray(0);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_MULTISAMPLE);
}

void MyTransform::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    shaderProgram.bind();
    vao.bind();

    //变换矩阵
    QMatrix4x4 transform;
    //向右下角平移
    transform.translate(QVector3D(0.5f, -0.5f, 0.0f));
    //设置
    shaderProgram.setUniformValue("color", QVector3D(1.0, 0, 0));
    shaderProgram.setUniformValue("transform", projection * transform);
    //根据索引绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

    //缩放
    transform.scale(0.5);
    //设置
    shaderProgram.setUniformValue("color", QVector3D(0, 1.0, 0));
    shaderProgram.setUniformValue("transform", projection * transform);
    //根据索引绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

    //绕z轴旋转
    transform.rotate(20, QVector3D(0.0f, 0.0f, 1.0f));
    //设置
    shaderProgram.setUniformValue("color", QVector3D(0, 0, 1.0));
    shaderProgram.setUniformValue("transform", projection * transform);
    //根据索引绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void MyTransform::resizeGL(int width, int height)
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
