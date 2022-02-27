#include "GLTexture330.h"
#include <QImage>
#include <QDebug>

GLTexture330::GLTexture330(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTexture330::~GLTexture330()
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
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);
    doneCurrent();
}

void GLTexture330::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部
    //--这里对纹理坐标的y进行的取反
    const char *vertex_str=R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
out vec3 theColor;
out vec2 texCoord;
void main() {
gl_Position = vec4(inPos,1.0);
theColor = inColor;
texCoord = vec2(inTexCoord.x, 1-inTexCoord.y);
})";
    const char *fragment_str=R"(#version 330 core
uniform sampler2D theTexture;
in vec3 theColor;
in vec2 texCoord;
out vec4 fragColor;
void main() {
fragColor = texture(theTexture, texCoord) * vec4(theColor, 1.0);
})";

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

    //从LearnOpenGL移植过来
    float vertices[] = {
        // positions          // colors           // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    //先设置顶点和索引
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //再设置纹理
    // load and create a texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    //GL_REPEAT重复，GL_MIRRORED_REPEAT镜像重复，GL_CLAMP_TO_EDGE拉伸，GL_CLAMP_TO_BORDER无重复
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    //GL_NEAREST临近值，GL_LINEAR线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    //参照博客 https://www.jianshu.com/p/273b7f960f3d
    QImage img1 = QImage(":/awesomeface.png").convertToFormat(QImage::Format_RGBA8888);
    if (!img1.isNull()) {
        // void glTexImage2D(GLenum target​, GLint level​, GLint internalFormat​, GLsizei width​, GLsizei height​, GLint border​, GLenum format​, GLenum type​, const GLvoid * data​);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img1.width(), img1.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img1.bits());
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void GLTexture330::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //深度测试发生于混合之前
    //当我们需要绘制透明图片时，就需要关闭GL_DEPTH_TEST并且打开混合glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //指定混合函数
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //安装所指定的程序对象程序作为当前再现状态的一部分
    glUseProgram(shaderProgram);

    //激活纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //根据索引渲染两个三角
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GLTexture330::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}

void GLTexture330::checkShaderError(GLuint id, const QString &type)
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
