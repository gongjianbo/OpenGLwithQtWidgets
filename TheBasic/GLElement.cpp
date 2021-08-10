#include "GLElement.h"

GLElement::GLElement(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLElement::~GLElement()
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
    ebo.destroy();
    doneCurrent();
}

void GLElement::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 450
layout (location = 0) in vec3 vertices;
void main() {
gl_Position = vec4(vertices,1.0);
})";
    const char *fragment_str=R"(#version 450
uniform vec3 myColor;
out vec4 fragColor;
void main() {
fragColor = vec4(myColor,1.0);
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

    //两个三角组合为矩形
    float vertices[] = {
        0.5f,  0.5f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    vao.create();
    vao.bind();

    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices)); //顶点数据

    ebo=QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ebo.create();
    ebo.bind();
    ebo.allocate(indices,sizeof(indices)); //索引数据


    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    int attr=0;
    attr=shaderProgram.attributeLocation("vertices");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    shaderProgram.enableAttributeArray(attr);

    vao.release();
    vbo.release();
    //当目标是GL_ELEMENT_ARRAY_BUFFER的时候，VAO会储存glBindBuffer的函数调用。
    //这也意味着它也会储存解绑调用，所以确保你没有在解绑VAO之前解绑索引数组缓冲，否则它就没有这个EBO配置了。
    ebo.release();
}

void GLElement::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProgram.bind();
    shaderProgram.setUniformValue("myColor",QVector3D(0.0f,1.0f,1.0f));
    {
        QOpenGLVertexArrayObject::Binder vao_bind(&vao); Q_UNUSED(vao_bind);
        //根据索引渲染
        //void glDrawElements(GLenum mode​, GLsizei count​,
        //                    GLenum type​, const GLvoid * indices​);
        //参数1图元类型
        //参数2索引个数
        //参数3索引数据类型
        //参数4指定一个字节偏移量（转换为指针类型）到绑定到GL_ELEMENT_ARRAY_BUFFER的缓冲区中以开始读取索引
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    //只有一个着色器，可以不用release
    shaderProgram.release();
}

void GLElement::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}
