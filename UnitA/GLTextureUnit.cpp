#include "GLTextureUnit.h"

GLTextureUnit::GLTextureUnit(QWidget *parent)
    : QOpenGLWidget(parent)
{

}

GLTextureUnit::~GLTextureUnit()
{
    makeCurrent();
    _vbo.destroy();
    _ebo.destroy();
    _vao.destroy();
    delete _texture1;
    delete _texture2;
    doneCurrent();
}

void GLTextureUnit::initializeGL()
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
                             uniform sampler2D texture1;
                             uniform sampler2D texture2;

                             void main()
                             {
                             gl_FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
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

    //VAO,VBO
    float vertices[] = {
        // positions          // colors           // texture coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };
    //EBO
    unsigned int indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    _vao.create();
    _vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&_vao);
    _vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    _vbo.create();
    _vbo.bind();
    _vbo.allocate(vertices,sizeof(vertices));

    _ebo=QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    _ebo.create();
    _ebo.bind();
    _ebo.allocate(indices,sizeof(indices));

    // position attribute
    int attr = -1;
    attr = _shaderProgram.attributeLocation("aPos");
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    _shaderProgram.enableAttributeArray(attr);
    // color attribute
    attr = _shaderProgram.attributeLocation("aColor");
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8);
    _shaderProgram.enableAttributeArray(attr);
    // texture coord attribute
    attr = _shaderProgram.attributeLocation("aTexCoord");
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8);
    _shaderProgram.enableAttributeArray(attr);

    // texture 1
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    _texture1 = new QOpenGLTexture(QImage(":/box.jpg"), QOpenGLTexture::GenerateMipMaps);
    if(!_texture1->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _texture1->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    _texture1->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    // set texture filtering parameters
    //等价于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _texture1->setMinificationFilter(QOpenGLTexture::Linear);
    _texture1->setMagnificationFilter(QOpenGLTexture::Linear);

    // texture 2
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    _texture2 = new QOpenGLTexture(QImage(":/face.png"), QOpenGLTexture::GenerateMipMaps);
    if(!_texture2->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _texture2->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    _texture2->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    // set texture filtering parameters
    //等价于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _texture2->setMinificationFilter(QOpenGLTexture::Linear);
    _texture2->setMagnificationFilter(QOpenGLTexture::Linear);

    _shaderProgram.bind();
    _shaderProgram.setUniformValue("texture1", 0);
    _shaderProgram.setUniformValue("texture2", 1);
    _shaderProgram.release();
}

void GLTextureUnit::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //当我们需要绘制透明图片时，就需要关闭GL_DEPTH_TEST并且打开混合glEnable(GL_BLEND);
    //glDisable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //基于源像素Alpha通道值的半透明混合函数
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    //纹理单元的应用
    glActiveTexture(GL_TEXTURE0);
    _texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    _texture2->bind();

    _shaderProgram.bind();
    //绘制
    //QOpenGLVertexArrayObject::Binder vaoBind(&_vao);
    _vao.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    _shaderProgram.release();
}

void GLTextureUnit::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
