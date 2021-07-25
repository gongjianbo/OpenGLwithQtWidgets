#include "GLTriangle.h"
#include <QDebug>

GLTriangle::GLTriangle(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTriangle::~GLTriangle()
{
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    doneCurrent();
}

void GLTriangle::initializeGL()
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

    //【0】有点没懂得是为什么着色器是create*来创建而不是gen*
    //顶点着色器
    //创建着色器对象
    //GLuint glCreateShader(GLenum shaderType​);
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    //设置着色器对象中的的代码
    //void glShaderSource(GLuint shader​, GLsizei count​,
    //                    const GLchar **string​, const GLint *length​);
    //参数1指定着色器对象
    //参数2指定字符串个数
    //参数3指定字符串二维数组指针
    //参数4指定字符串长度数组，为NULL则以'\0'为字符串终止符
    glShaderSource(vertex_shader, 1, &vertex_str, NULL);
    //编译着色器对象
    //void glCompileShader(GLuint shader​);
    glCompileShader(vertex_shader);
    //检测着色器是否异常
    checkShaderError(vertex_shader, "vertex");

    //片段着色器
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_str, NULL);
    glCompileShader(fragment_shader);
    checkShaderError(fragment_shader,"fragment");

    //着色器程序
    //创建一个空的程序对象并返回一个可以被引用的非零值
    //GLuint glCreateProgram(void​);
    shaderProgram = glCreateProgram();
    //将着色器对象附加到程序对象
    //void glAttachShader(GLuint program​, GLuint shader​);
    glAttachShader(shaderProgram, vertex_shader);
    glAttachShader(shaderProgram, fragment_shader);
    //链接程序对象
    //void glLinkProgram(GLuint program​);
    glLinkProgram(shaderProgram);
    checkShaderError(shaderProgram, "program");
    //删除着色器，它们已经链接到我们的着色器程序对象了
    //void glDeleteShader(GLuint shader​);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    //三角形的三个顶点
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left
        0.5f, -0.5f, 0.0f, // right
        0.0f,  0.5f, 0.0f  // top
    };
    //【1】2014年8月12日，Khronos发布了OpenGL 4.5标准规范，
    //其中ARB_direct_state_access扩展进入核心，
    //其允许直接访问和修改OpenGL对象而无需绑定OpenGL对象（bind操作，例如glBindBuffer），
    //提高应用程序和中间件的效率。
    //【2】glGen*系列函数生成的id，内部并没有初始化那个对象的状态。只有到了glBind*的时候才会初始化。
    //而Core/ARB的DSA直接提供了glCreate*系列函数，可以一步到位地建立id和初始化。

    //生成顶点数组对象
    //void glCreateVertexArrays(GLsizei n, GLuint *arrays);
    glCreateVertexArrays(1, &vao);
    //生成缓冲区对象
    //void glCreateBuffers(GLsizei n, GLuint *buffers);
    glCreateBuffers(1, &vbo);
    //分配size个存储单元存储数据或索引
    //glNamedBufferStorage(GLuint buffer, GLsizeiptr size,
    //                     const void *data, GLbitfield flags);
    //参数1缓冲区对象
    //参数2数据块大小
    //参数3如果为NULL则是size个为初始化的数据，否则以data拷贝初始化
    //参数4数据相关的用途
    glNamedBufferStorage(vbo, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);
    //vbo绑定到vao
    //glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex,
    //                          GLuint buffer, GLintptr offset, GLsizei stride);
    //参数1顶点数组对象
    //参数2vbo在bao的索引
    //参数3顶点缓冲对象
    //参数4缓冲区第一个元素的偏移
    //参数5缓冲区顶点步进，三角形一个点3个float
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3*sizeof(float));
    //启用通用顶点 attribute 数组的 index 索引
    //glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
    glEnableVertexArrayAttrib(vao, 0);
    //指定顶点数组的组织
    //glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex,
    //                          GLint size, GLenum type,
    //                          GLboolean normalized, GLuint relativeoffset);
    //参数1顶点数组对象
    //参数2通用顶点 attribute 数组
    //参数3每个顶点几个数据
    //参数4存储类型
    //参数5是否归一化
    //参数6顶点步进
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    //关联顶点 attribute 属性和顶点缓冲区的绑定
    //glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex,
    //                           GLuint bindingindex);
    //参数1顶点数组对象
    //参数2属性 attribute 索引，对应layout
    //参数3vbo在vao的索引
    glVertexArrayAttribBinding(vao, 0, 0);
}

void GLTriangle::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //安装所指定的程序对象程序作为当前再现状态的一部分
    glUseProgram(shaderProgram);
    //传递值
    glUniform3f(glGetUniformLocation(shaderProgram, "myColor"), 0.0f, 1.0f, 0.0f);
    //绑定数组对象
    glBindVertexArray(vao);

    //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
    //void glDrawArrays(GLenum mode​, GLint first​, GLsizei count​);
    //参数1为图元类型
    //参数2指定顶点数组的起始索引
    //参数3指定顶点个数
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GLTriangle::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}

void GLTriangle::checkShaderError(GLuint id, const QString &type)
{
    int check_flag;
    char check_info[1024];
    if(type != "program"){
        glGetShaderiv(id, GL_COMPILE_STATUS, &check_flag);
        if(!check_flag){
            glGetShaderInfoLog(id, 1024, NULL, check_info);
            qDebug() << type << " error:" << check_info;
        }
    }else{
        glGetShaderiv(id, GL_LINK_STATUS, &check_flag);
        if(!check_flag){
            glGetProgramInfoLog(id, 1024, NULL, check_info);
            qDebug() << type << " error:" << check_info ;
        }
    }
}
