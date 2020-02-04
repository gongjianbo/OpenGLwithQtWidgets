#include "GLTestPoker.h"

#include <QObject>

GLTestPoker::GLTestPoker(QWidget *parent)
    : QOpenGLWidget(parent)
{
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,this,[this](){
        _rotate+=2;
        update();
    });
    _timer->setInterval(50);
}

GLTestPoker::~GLTestPoker()
{
    makeCurrent();
    _vbo.destroy();
    _vao.destroy();
    delete _texture;
    doneCurrent();
}

void GLTestPoker::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //因为OpenGL纹理颠倒过来的，所以取反vec2(aTexCoord.x, 1-aTexCoord.y);
    const char *vertex_str=R"(#version 330 core
                           layout (location = 0) in vec3 aPos;
                           layout (location = 1) in vec2 aTexCoord;
                           out vec2 TexCoord;
                           uniform mat4 model;
                           uniform mat4 view;
                           uniform mat4 projection;

                           void main()
                           {
                           gl_Position = projection * view * model * vec4(aPos, 1.0f);
                           TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
                           })";
    const char *fragment_str=R"(#version 330 core
                             in vec2 TexCoord;
                             uniform sampler2D texture1;

                             void main()
                             {
                             gl_FragColor = texture(texture1, TexCoord);
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

    //VAO,VBO（一个面两个三角）宽高比360/460
    const QImage img_temp(":/liuyifei.png");
    //上下两张图，所以高度除以二
    const float texture_width=1.0f*img_temp.width()/(img_temp.height()/2);
    float vertices[] = {
        -texture_width/2, -0.5f, 0.0f,  0.0f, 0.5f, //左下角
        texture_width/2, -0.5f, 0.0f,  1.0f, 0.5f, //右下角
        texture_width/2,  0.5f, 0.0f,  1.0f, 1.0f, //右上角

        texture_width/2,  0.5f, 0.0f,  1.0f, 1.0f, //右上角
        -texture_width/2,  0.5f, 0.0f,  0.0f, 1.0f, //左上角
        -texture_width/2, -0.5f, 0.0f,  0.0f, 0.5f, //左下角

        -texture_width/2, -0.5f, 0.0f,  1.0f, 0.0f, //左下角
        -texture_width/2,  0.5f, 0.0f,  1.0f, 0.5f, //左上角
        texture_width/2,  0.5f, 0.0f,  0.0f, 0.5f, //右上角

        texture_width/2,  0.5f, 0.0f,  0.0f, 0.5f, //右上角
        texture_width/2, -0.5f, 0.0f,  0.0f, 0.0f, //右下角
        -texture_width/2, -0.5f, 0.0f,  1.0f, 0.0f, //左下角
    };
    _vao.create();
    _vao.bind();
    _vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    _vbo.create();
    _vbo.bind();
    _vbo.allocate(vertices,sizeof(vertices));

    // position attribute
    int attr = -1;
    attr = _shaderProgram.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    _shaderProgram.enableAttributeArray(attr);
    // texture coord attribute
    attr = _shaderProgram.attributeLocation("aTexCoord");
    _shaderProgram.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    _shaderProgram.enableAttributeArray(attr);

    // texture 1
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    _texture = new QOpenGLTexture(img_temp, QOpenGLTexture::GenerateMipMaps);
    if(!_texture->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _texture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    _texture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    // set texture filtering parameters
    //等价于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _texture->setMinificationFilter(QOpenGLTexture::Linear);
    _texture->setMagnificationFilter(QOpenGLTexture::Linear);

    _shaderProgram.bind();
    _shaderProgram.setUniformValue("texture1", 0);
    _shaderProgram.release();

    _timer->start();
}

void GLTestPoker::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE); //剔除
    glCullFace(GL_BACK); //剔除背面
    //glFrontFace(GL_CW); //默认逆时针顶点为正面GL_CCW

    glActiveTexture(GL_TEXTURE0);
    _texture->bind();

    _shaderProgram.bind();
    _vao.bind();

    QMatrix4x4 view; //观察矩阵，后退一点
    view.translate(QVector3D(0.0f, 0.0f, -1.5f));
    _shaderProgram.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    _shaderProgram.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    model.rotate(_rotate, QVector3D(0.0f, 1.0f, 0.0f));
    _shaderProgram.setUniformValue("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 12);

    _vao.release();
    _texture->release();
    _shaderProgram.release();
}

void GLTestPoker::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
