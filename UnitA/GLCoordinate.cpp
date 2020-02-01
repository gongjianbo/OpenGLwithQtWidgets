#include "GLCoordinate.h"

#include <QDebug>

GLCoordinate::GLCoordinate(QWidget *parent)
    : QOpenGLWidget(parent)
{
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,this,[this](){
        _rotate+=2;
        update();
    });
    _timer->setInterval(50);
}

GLCoordinate::~GLCoordinate()
{
    makeCurrent();
    _vbo.destroy();
    _vao.destroy();
    delete _texture1;
    delete _texture2;
    doneCurrent();
}

void GLCoordinate::initializeGL()
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
                             out vec4 FragColor;
                             in vec2 TexCoord;
                             uniform sampler2D texture1;
                             uniform sampler2D texture2;

                             void main()
                             {
                             FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
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

    //VAO,VBO（盒子六个面，一个面两个三角）
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    _vao.create();
    _vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&_vao);
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

    QMatrix4x4 view; //观察矩阵，后退一点
    view.translate(QVector3D(0.0f, 0.0f, -3.0f));
    _shaderProgram.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    //坐标到达观察空间之后，我们需要将其投影到裁剪坐标。
    //裁剪坐标会被处理至-1.0到1.0的范围内，并判断哪些顶点将会出现在屏幕上
    //参数1：指定视景体的视野的角度
    //参数2：指定你的视景体的宽高比
    //参数3：指定观察者到视景体的最近的裁剪面的距离（正数）
    //参数4：指定观察者到视景体最远的裁剪面距离（正数）
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    _shaderProgram.setUniformValue("projection", projection);
    _shaderProgram.release();

    _timer->start();
}

//绘制多个盒子
static QVector3D cubePositions[] = {
    QVector3D( 0.0f,  0.0f,  0.0f),
    QVector3D( 2.0f,  5.0f, -15.0f),
    QVector3D(-1.5f, -2.2f, -2.5f),
    QVector3D(-3.8f, -2.0f, -12.3f),
    QVector3D( 2.4f, -0.4f, -3.5f),
    QVector3D(-1.7f,  3.0f, -7.5f),
    QVector3D( 1.3f, -2.0f, -2.5f),
    QVector3D( 1.5f,  2.0f, -2.5f),
    QVector3D( 1.5f,  0.2f, -1.5f),
    QVector3D(-1.3f,  1.0f, -1.5f)
};

void GLCoordinate::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //因为我们使用了深度测试，我们也想要在每次渲染迭代之前清除深度缓冲
    //（否则前一帧的深度信息仍然保存在缓冲中）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //OpenGL存储它的所有深度信息于一个Z缓冲(Z-buffer)中，也被称为深度缓冲(Depth Buffer)。
    //深度值存储在每个片段里面（作为片段的z值），当片段想要输出它的颜色时，OpenGL会将它的深度值和z缓冲进行比较，
    //如果当前的片段在其它片段之后，它将会被丢弃，否则将会覆盖。
    //这个过程称为深度测试(Depth Testing)，它是由OpenGL自动完成的。
    //（不开启深度缓冲的话，盒子的纹理堆叠顺序就是乱的）
    glEnable(GL_DEPTH_TEST); //默认关闭的

    //纹理单元的应用
    glActiveTexture(GL_TEXTURE0);
    _texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    _texture2->bind();

    _shaderProgram.bind();
    _vao.bind();
    for (unsigned int i = 0; i < 10; i++) {
        //计算模型矩阵
        QMatrix4x4 model;
        //平移
        model.translate(cubePositions[i]);
        //这样每个箱子旋转的速度就不一样
        float angle = (i + 1.0f) * _rotate;
        //旋转
        model.rotate(angle, QVector3D(1.0f, 0.3f, 0.5f));
        //传入着色器并绘制
        _shaderProgram.setUniformValue("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    _vao.release();
    _texture1->release();
    _texture2->release();
    _shaderProgram.release();
}

void GLCoordinate::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
