#include "GLDepthTesting.h"

GLDepthTesting::GLDepthTesting(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLDepthTesting::~GLDepthTesting()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    doneCurrent();
}

void GLDepthTesting::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 450
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
uniform mat4 view;
uniform mat4 projection;
out vec3 theColor;
void main() {
gl_Position = projection * view * vec4(vPos, 1.0f);
theColor = vColor;
})";
    //z深度值越大，颜色越白
    const char *fragment_str=R"(#version 450
in vec3 theColor;
out vec4 fragColor;
void main() {
fragColor = vec4(theColor * (1.0f - gl_FragCoord.z), 1.0f);
})";

    //顶点着色器
    //可以直接add着色器代码，也可以借助QOpenGLShader类
    bool success=shaderProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,vertex_str);
    if(!success){
        qDebug()<<"compiler vertex failed!"<<shaderProgram.log();
    }
    //片段着色器
    success=shaderProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,fragment_str);
    if(!success){
        qDebug()<<"compiler fragment failed!"<<shaderProgram.log();
    }
    success = shaderProgram.link();
    if(!success){
        qDebug()<<"link shader failed!"<<shaderProgram.log();
    }

    //两个交叠的三角形顶点
    //这里的z值和投影的z范围对应
    GLfloat vertices[] = {
        -0.55f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.55f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, -50.0f, 1.0f, 0.0f, 0.0f,

        0.55f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.55f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, 0.0f, -50.0f, 0.0f, 0.0f, 1.0f
    };

    vao.create();
    vao.bind();
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    shaderProgram.enableAttributeArray(0);
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    shaderProgram.enableAttributeArray(1);
    vbo.release();
    vao.release();

    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //深度测试默认是关闭的，需要用GL_DEPTH_TEST选项启用深度测试
    glEnable(GL_DEPTH_TEST);
    //在某些情况下我们需要进行深度测试并相应地丢弃片段，但我们不希望更新深度缓冲区，
    //基本上，可以使用一个只读的深度缓冲区；
    //OpenGL允许我们通过将其深度掩码设置为GL_FALSE禁用深度缓冲区写入:
    //glDepthMask(GL_FALSE);
    //可以修改深度测试使用的比较规则，默认GL_LESS丢弃深度大于等于的
    //glDepthFunc(GL_LESS);
    //使近平面外的可见
    //glEnable(GL_DEPTH_CLAMP);
}

void GLDepthTesting::paintGL()
{
    //渲染之前使用GL_DEPTH_BUFFER_BIT清除深度缓冲区
    //否则深度缓冲区将保留上一次进行深度测试时所写的深度值
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shaderProgram.bind();
    //观察矩阵
    QMatrix4x4 view;
    //OpenGL本身没有摄像机(Camera)的概念，但我们可以通过把场景中的所有物体往相反方向移动的方式来模拟出摄像机，
    //产生一种我们在移动的感觉，而不是场景在移动。
    view.translate(QVector3D(0.0f, 0.0f, -3.0f));
    shaderProgram.setUniformValue("view", view);
    //投影矩阵
    shaderProgram.setUniformValue("projection", projection);
    {
        QOpenGLVertexArrayObject::Binder vao_bind(&vao); Q_UNUSED(vao_bind);
        //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
        //void glDrawArrays(GLenum mode​, GLint first​, GLsizei count​);
        //参数1为图元类型
        //参数2指定顶点数组的起始索引
        //参数3指定顶点个数
        //（这里分别画两个三角）
        glDrawArrays(GL_TRIANGLES, 0, 3); //red
        glDrawArrays(GL_TRIANGLES, 3, 3); //blue
        //glDrawArrays(GL_TRIANGLES, 0, 3); //red
    }
    //只有一个着色器，可以不用release
    shaderProgram.release();
}

void GLDepthTesting::resizeGL(int width, int height)
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
