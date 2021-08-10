#include "GLTriangle330.h"
#include <QDebug>

GLTriangle330::GLTriangle330(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTriangle330::~GLTriangle330()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    doneCurrent();
}

void GLTriangle330::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 330 core
layout (location = 0) in vec3 vertices;
void main() {
gl_Position = vec4(vertices,1.0);
})";
    const char *fragment_str=R"(#version 330 core
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
    //【1】状态机模式需要先绑定，后面对应的操作就是针对这个对象进行的
    //顶点数组对象（VAO）可以像顶点缓冲对象（VBO）那样被绑定，
    //任何随后的顶点属性调用都会储存在这个VAO中。
    //这样的好处就是，当配置顶点属性指针时，你只需要将那些调用执行一次，
    //之后再绘制物体的时候只需要绑定相应的VAO就行了，所有状态都将存储在VAO中。
    //这使在不同顶点数据和属性配置之间切换变得非常简单，只需要绑定不同的VAO就行了，
    //不用重复绑定一堆VBO去操作。
    //【2】glGen*系列函数生成的id，内部并没有初始化那个对象的状态。只有到了glBind*的时候才会初始化。
    //而Core/ARB的DSA直接提供了glCreate*系列函数，可以一步到位地建立id和初始化。

    //生成顶点数组对象
    //void glGenVertexArrays(GLsizei n​, GLuint *arrays​);
    glGenVertexArrays(1, &vao);
    //绑定顶点数组对象
    //void glBindVertexArray(GLuint array​);
    glBindVertexArray(vao);

    //生成缓冲区对象
    //void glGenBuffers(GLsizei n​, GLuint * buffers​);
    glGenBuffers(1, &vbo);
    //绑定缓冲区对象
    //void glBindBuffer(GLenum target​, GLuint buffer​);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    //将顶点数据复制到缓冲的内存中
    //glBufferData是一个专门用来把用户定义的数据复制到当前绑定缓冲的函数
    //void glBufferData(GLenum target​, GLsizeiptr size​, const GLvoid * data​, GLenum usage​);
    //参数1缓冲对象类型，参数2数据字节大小，参数3数据指针，没数据则为NULL
    //参数4指定了我们希望显卡如何管理给定的数据，他有三种形式：
    //GL_STATIC_DRAW/READ/COPY ：数据不会或几乎不会改变。
    //GL_DYNAMIC_DRAW/READ/COPY：数据会被改变很多。
    //GL_STREAM_DRAW/READ/COPY ：数据每次绘制时都会改变。
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //定义通用顶点属性数据的数组
    //void glVertexAttribPointer(GLuint index​, GLint size​, GLenum type​,
    //    GLboolean normalized​, GLsizei stride​, const GLvoid * pointer​);
    //参数1指定要配置的通用顶点属性的索引，对应顶点着色器中的（layout(location = 0)）
    //参数2指定顶点属性的大小，1-4，这里顶点属性是vec3，所以填3
    //参数3指定数据类型
    //参数4定义我们是否希望数据被标准化，为true则数据被归一化0-1
    //参数5为字节步长，告诉我们在连续的顶点属性组之间的间隔
    //由于下个组位于3个float之后，所以置为3 * sizeof(float)
    //参数6表示位置数据在缓冲中起始位置的偏移量(Offset)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    //使能顶点属性数组
    //void glEnableVertexAttribArray(GLuint index​);
    //参数为顶点属性的索引
    glEnableVertexAttribArray(0);
    //由于是状态机模式
    //对glVertexAttribPointer的调用将VBO注册为顶点属性的绑定顶点缓冲区对象

    //解绑VB0，bind到0上
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //解绑VAO
    glBindVertexArray(0);
}

void GLTriangle330::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //安装所指定的程序对象程序作为当前再现状态的一部分
    //void glUseProgram(GLuint program​);
    glUseProgram(shaderProgram);
    //传递值
    glUniform3f(glGetUniformLocation(shaderProgram, "myColor"), 0.0f, 1.0f, 0.0f);
    //绑定数组对象
    //void glBindVertexArray(GLuint array​);
    glBindVertexArray(vao);

    //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
    //void glDrawArrays(GLenum mode​, GLint first​, GLsizei count​);
    //参数1为图元类型
    //参数2指定顶点数组的起始索引
    //参数3指定顶点个数
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GLTriangle330::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}

void GLTriangle330::checkShaderError(GLuint id, const QString &type)
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
