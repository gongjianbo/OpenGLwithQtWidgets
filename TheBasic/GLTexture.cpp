#include "GLTexture.h"
#include <QImage>
#include <QDebug>

GLTexture::GLTexture(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTexture::~GLTexture()
{
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    //释放的时候，如果该部件处于非当前显示的tab页，会异常退出
    //（使用qt封装的类没有出现退出异常）
    //单独使用时去掉注释
    /*glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);*/
    doneCurrent();
}

void GLTexture::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //OpenGL要求y轴0.0坐标是在图片的底部的，但是图片的y轴0.0坐标通常在顶部
    //--这里对纹理坐标的y进行的取反
    const char *vertex_str=R"(#version 450
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
    const char *fragment_str=R"(#version 450
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
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,  0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    //生成顶点数组对象
    //void glCreateVertexArrays(GLsizei n, GLuint *arrays);
    glCreateVertexArrays(1, &vao);
    //生成顶点缓冲对象
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
    //参数2vbo在vao的索引
    //参数3顶点缓冲对象
    //参数4缓冲区第一个元素的偏移
    //参数5缓冲区顶点步进，三角形一个点3个float
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 8*sizeof(float));
    //启用通用顶点 attribute 数组的 index 索引，对应layout location
    //glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glEnableVertexArrayAttrib(vao, 2);
    //指定顶点数组的组织
    //glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex,
    //                          GLint size, GLenum type,
    //                          GLboolean normalized, GLuint relativeoffset);
    //参数1顶点数组对象
    //参数2通用顶点 attribute 数组，对应layout location
    //参数3每个顶点几个数据
    //参数4存储类型
    //参数5是否归一化
    //参数6顶点步进
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float));
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float));
    //关联顶点 attribute 属性和顶点缓冲区的绑定
    //glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex,
    //                           GLuint bindingindex);
    //参数1顶点数组对象
    //参数2属性 attribute 索引，对应layout location
    //参数3vbo在vao的索引
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribBinding(vao, 2, 0);

    //生成索引缓冲对象
    glCreateBuffers(1, &ebo);
    //分配size个存储单元存储数据或索引
    glNamedBufferStorage(ebo, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);
    //ebo绑定到vao
    //void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer);
    glVertexArrayElementBuffer(vao, ebo);

    //图片读取
    QImage img(":/awesomeface.png");
    img.convertTo(QImage::Format_RGBA8888);
    //创建纹理对象
    //void glCreateTextures(GLenum target, GLsizei n, GLuint *textures);
    glCreateTextures(GL_TEXTURE_2D,1,&texture);
    //设置纹理参数
    //void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
    //GL_TEXTURE_WRAP设置纹理环绕方式，即超出纹理坐标怎么处理
    //s\t\r对应x\y\z
    //GL_REPEAT重复，GL_MIRRORED_REPEAT镜像重复，GL_CLAMP_TO_EDGE拉伸，GL_CLAMP_TO_BORDER无重复
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //纹理过滤，怎样将纹理像素(Texture Pixel)映射到纹理坐标
    //MIN对应Minify缩小，MAG对应Magnify放大
    //GL_NEAREST临近值，GL_LINEAR线性插值
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //定义贴图的使用内存大小
    //void glTextureStorage2D(GLuint texture, GLsizei levels,
    //                        GLenum internalformat, GLsizei width, GLsizei height);
    //参数1纹理对象
    //参数2levels和多级渐远相关
    //参数3存储格式
    //参数4宽度
    //参数5高度
    glTextureStorage2D(texture,1,GL_RGBA8,img.width(),img.height());
    //加载纹理数据
    //void glTextureSubImage2D(GLuint texture, GLint level,
    //                         GLint xoffset, GLint yoffset,
    //                         GLsizei width, GLsizei height, GLenum format,
    //                         GLenum type, const void *pixels);
    //参数1纹理对象
    //参数2levels和多级渐远相关
    //参数3纹理阵列中x方向上的纹理偏移
    //参数4纹理阵列中y方向上的纹理偏移
    //参数5纹理图像宽
    //参数6纹理图像高
    //参数7图像格式
    //参数8像素数据类型
    //参数9数据指针
    glTextureSubImage2D(texture,0,0,0,img.width(),img.height(),GL_RGBA,GL_UNSIGNED_BYTE,img.bits());
}

void GLTexture::paintGL()
{
    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //当我们需要绘制透明图片时，就需要关闭GL_DEPTH_TEST并且打开混合glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //基于源像素Alpha通道值的半透明混合函数
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    //安装所指定的程序对象程序作为当前再现状态的一部分
    glUseProgram(shaderProgram);

    //激活纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GLTexture::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}

void GLTexture::checkShaderError(GLuint id, const QString &type)
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
