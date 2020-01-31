#include "GLTexture.h"

#include <QDebug>

GLTexture::GLTexture(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTexture::~GLTexture()
{
    glDeleteVertexArrays(1, &_VAO);
    glDeleteBuffers(1, &_VBO);
    glDeleteBuffers(1, &_EBO);
    glDeleteTextures(1,&_Texture);
}

void GLTexture::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //因为OpenGL纹理颠倒过来的，所以取反vec2(aTexCoord.x, 1-aTexCoord.y);
    const char *vertex_str=R"(#version 330 core
                           layout (location = 0) in vec3 aPos;
                           layout (location = 1) in vec3 aColor;
                           layout (location = 2) in vec2 aTexCoord;
                           out vec3 ourColor;
                           out vec2 TexCoord;

                           void main()
                           {
                           gl_Position = vec4(aPos, 1.0);
                           ourColor = aColor;
                           TexCoord = vec2(aTexCoord.x, 1-aTexCoord.y);
                           })";
    const char *fragment_str=R"(#version 330 core
                             in vec3 ourColor;
                             in vec2 TexCoord;
                             uniform sampler2D ourTexture;

                             void main()
                             {
                             gl_FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);
                             })";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!_shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<_shaderProgram.log();
    }
    if(!_shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<_shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!_shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<_shaderProgram.log();
    }

    //从LearnOpenGL移植过来
    //纹理坐标x范围增大了，使它看起来像正方形
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
    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &_VBO);
    glGenBuffers(1, &_EBO);

    glBindVertexArray(_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _EBO);
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


    // load and create a texture
    glGenTextures(1, &_Texture);
    glBindTexture(GL_TEXTURE_2D, _Texture);
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
    QImage img1 = QImage(":/face.png").convertToFormat(QImage::Format_RGBA8888);
    if (!img1.isNull()) {
        // void glTexImage2D(GLenum target​, GLint level​, GLint internalFormat​, GLsizei width​, GLsizei height​, GLint border​, GLenum format​, GLenum type​, const GLvoid * data​);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img1.width(), img1.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img1.bits());
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    // don't forget to activate/use the shader before setting uniforms!
    //只有一个纹理贴图可以不用，默认就是0
    //_shaderProgram.bind();
    //glUniform1i(_shaderProgram.uniformLocation("texture1"), 0);
    //_shaderProgram.release();
}

void GLTexture::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //当我们需要绘制透明图片时，就需要关闭GL_DEPTH_TEST并且打开混合glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //基于源像素Alpha通道值的半透明混合函数
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _Texture);

    _shaderProgram.bind();
    //绘制
    glBindVertexArray(_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    _shaderProgram.release();
}

void GLTexture::resizeGL(int width, int height)
{
    glViewport(0,0,width,height);
}
