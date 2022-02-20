#include "GLFaceCulling.h"

GLFaceCulling::GLFaceCulling(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLFaceCulling::~GLFaceCulling()
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

void GLFaceCulling::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 450
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
uniform mat4 mvp;
out vec3 theColor;
void main() {
gl_Position = mvp * vec4(vPos, 1.0f);
theColor = vColor;
})";
    const char *fragment_str=R"(#version 450
in vec3 theColor;
out vec4 fragColor;
void main() {
fragColor = vec4(theColor, 1.0f);
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

    //两个不同环绕顺序、不同颜色的三角形
    GLfloat vertices[] = {
        //左侧顺时针-红色
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        //右侧逆时针-蓝色
        0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f
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
}

void GLFaceCulling::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //使能面剔除
    glEnable(GL_CULL_FACE);
    //GL_FRONT剔除正面，GL_BACK剔除背面(默认)，GL_FRONT_AND_BACK剔除正反面
    //glCullFace(GL_FRONT);
    //设置正面的环绕方式，GL_CCW逆时针(默认)，GL_CW顺时针
    //glFrontFace(GL_CW);

    shaderProgram.bind();
    //观察矩阵
    QMatrix4x4 view;
    //OpenGL本身没有摄像机(Camera)的概念，但我们可以通过把场景中的所有物体往相反方向移动的方式来模拟出摄像机，
    //产生一种我们在移动的感觉，而不是场景在移动。
    view.translate(QVector3D(0.0f, 0.0f, -3.0f));
    shaderProgram.setUniformValue("mvp", projection * view);
    {
        QOpenGLVertexArrayObject::Binder vao_bind(&vao); Q_UNUSED(vao_bind);
        //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
        //void glDrawArrays(GLenum mode​, GLint first​, GLsizei count​);
        //参数1为图元类型
        //参数2指定顶点数组的起始索引
        //参数3指定顶点个数
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    shaderProgram.release();
}

void GLFaceCulling::resizeGL(int width, int height)
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
