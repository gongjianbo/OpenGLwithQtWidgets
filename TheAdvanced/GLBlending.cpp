#include "GLBlending.h"

GLBlending::GLBlending(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLBlending::~GLBlending()
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

void GLBlending::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 450
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec4 vColor;
uniform mat4 mvp;
out vec4 theColor;
void main() {
gl_Position = mvp * vec4(vPos, 1.0f);
theColor = vColor;
})";
    //discard可以直接丢弃该片段，但这样没法处理半透明
    const char *fragment_str=R"(#version 450
in vec4 theColor;
out vec4 fragColor;
void main() {
//if(theColor.a < 1.0){
//discard;
//}
fragColor = theColor;
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
    GLfloat vertices[] = {
        //底部红色
        -0.55f, 1.0f, -10.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.55f, -1.0f, -10.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, -10.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        //表面蓝色
        0.55f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,
        0.55f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,
        -1.0f, 0.0f, -50.0f, 0.0f, 0.0f, 1.0f, 0.5f
    };

    vao.create();
    vao.bind();
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 7);
    shaderProgram.enableAttributeArray(0);
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 4, sizeof(GLfloat) * 7);
    shaderProgram.enableAttributeArray(1);
    vbo.release();
    vao.release();

    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void GLBlending::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //使能混合模式
    //混合操作发生在源颜色和目标颜色之间。
    //源颜色是片段着色器写入的值。目标颜色是帧缓冲区中图像的颜色。
    glEnable(GL_BLEND);
    //混合参数
    //void glBlendFunc(GLenum sfactor​, GLenum dfactor​);
    //sfactor​源混合因子，初始值GL_ONE
    //dfactor​目标混合因子，初始值GL_ZERO
    //GL_ZERO，表示使用0.0作为因子，实际上相当于不使用这种颜色参与混合运算。
    //GL_ONE，表示使用1.0作为因子，实际上相当于完全的使用了这种颜色参与混合运算。
    //GL_SRC_ALPHA，表示使用源颜色的alpha值来作为因子。
    //GL_DST_ALPHA，表示使用目标颜色的alpha值来作为因子。
    //GL_ONE_MINUS_SRC_ALPHA，表示用1.0减去源颜色的alpha值来作为因子。
    //GL_ONE_MINUS_DST_ALPHA，表示用1.0减去目标颜色的alpha值来作为因子。
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //混合方式
    //GL_FUNC_ADD：默认选项，将两个分量相加
    //GL_FUNC_SUBTRACT：源减去目标分量
    //GL_FUNC_REVERSE_SUBTRACT：目标减去源分量
    //glBlendEquation(GL_FUNC_ADD);

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
        //【】
        //当绘制一个有不透明和透明物体的场景的时候，大体的原则如下：
        //1.先绘制所有不透明的物体
        //2.对所有透明的物体排序
        //3.按顺序绘制所有透明的物体
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawArrays(GL_TRIANGLES, 3, 3);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    shaderProgram.release();
}

void GLBlending::resizeGL(int width, int height)
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
