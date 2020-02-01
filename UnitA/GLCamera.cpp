#include "GLCamera.h"

#include <QDebug>

GLCamera::GLCamera(QWidget *parent)
    : QOpenGLWidget(parent)
{
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,this,[this](){
        _rotate+=2;
        update();
    });
    _timer->setInterval(50);

    setFocusPolicy(Qt::ClickFocus); //默认没有焦点
    calculateCamera();
}

GLCamera::~GLCamera()
{
    makeCurrent();
    _vbo.destroy();
    _vao.destroy();
    delete _texture1;
    delete _texture2;
    doneCurrent();
}

void GLCamera::initializeGL()
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

void GLCamera::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //每次渲染迭代之前清除深度缓冲，否则前一帧的深度信息仍然保存在缓冲中
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer)，也被称为深度缓冲(Depth Buffer)
    glEnable(GL_DEPTH_TEST);

    //纹理单元的应用
    glActiveTexture(GL_TEXTURE0);
    _texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    _texture2->bind();

    _shaderProgram.bind();
    _vao.bind();

    /*//观察矩阵
    QMatrix4x4 view;
    //3个相互垂直的轴和一个定义摄像机空间的位置坐标
    //第一组参数：eyex, eyey,eyez 相机在世界坐标的位置
    //第二组参数：centerx,centery,centerz 相机镜头对准的物体在世界坐标的位置
    //第三组参数：upx,upy,upz 相机向上的方向在世界坐标中的方向，(0,-1,0)就旋转了190度
    view.lookAt(_cameraPosition,
                _cameraPosition+_cameraFront,
                _cameraUp);
    _shaderProgram.setUniformValue("view", view);*/
    _shaderProgram.setUniformValue("view", getViewMatrix());
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(_Zoom, 1.0f * width() / height(), 0.1f, 100.0f);
    _shaderProgram.setUniformValue("projection", projection);

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

void GLCamera::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLCamera::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_W: //上
        _cameraPosition-=_cameraWorldUp * _Speed;
        break;
    case Qt::Key_S://下
        _cameraPosition+=_cameraWorldUp * _Speed;
        break;
    case Qt::Key_A: //左
        _cameraPosition+=_cameraRight * _Speed;
        break;
    case Qt::Key_D: //右
        _cameraPosition-=_cameraRight * _Speed;
        break;
    case Qt::Key_E://近
        _cameraPosition+=_cameraFront * _Speed;
        break;
    case Qt::Key_Q: //远
        _cameraPosition-=_cameraFront * _Speed;
        break;
    default:
        break;
    }
    /*switch (event->key()) {
    case Qt::Key_W: //前
        _cameraPosition+=_cameraFront*0.5f;
        break;
    case Qt::Key_S: //后
        _cameraPosition-=_cameraFront*0.5f;
        break;
    case Qt::Key_A: //左
        _cameraPosition-=QVector3D::crossProduct(_cameraFront, _cameraUp)
                .normalized()*0.5f;
        break;
    case Qt::Key_D: //右
        _cameraPosition+=QVector3D::crossProduct(_cameraFront, _cameraUp)
                .normalized()*0.5f;
        break;
    default:
        break;
    }*/
    update();
    QWidget::keyPressEvent(event);
}

void GLCamera::mousePressEvent(QMouseEvent *event)
{
    _mousePos=event->pos();
    QWidget::mousePressEvent(event);
}

void GLCamera::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void GLCamera::mouseMoveEvent(QMouseEvent *event)
{
    int x_offset=event->pos().x()-_mousePos.x();
    int y_offset=event->pos().y()-_mousePos.y();
    _mousePos=event->pos();
    _Yaw -= x_offset*_Sensitivity;
    _Pitch += y_offset*_Sensitivity;

    if (_Pitch > 89.0f)
        _Pitch = 89.0f;
    else if (_Pitch < -89.0f)
        _Pitch = -89.0f;
    calculateCamera();
    update();
    QWidget::mouseMoveEvent(event);
}

void GLCamera::wheelEvent(QWheelEvent *event)
{
    if(event->delta()<0){
        _Zoom+=_Speed;
        if(_Zoom>90)
            _Zoom=90;
    }else{
        _Zoom-=_Speed;
        if(_Zoom<1)
            _Zoom=1;
    }
    update();
    QWidget::wheelEvent(event);
}

void GLCamera::calculateCamera()
{
    QVector3D front;
    front.setX(cos(_Yaw) * cos(_Pitch));
    front.setY(sin(_Pitch));
    front.setZ(sin(_Yaw) * cos(_Pitch));
    _cameraFront = front.normalized();
    _cameraRight = QVector3D::crossProduct(_cameraFront, _cameraWorldUp).normalized();
    _cameraUp = QVector3D::crossProduct(_cameraRight, _cameraFront).normalized();
}

QMatrix4x4 GLCamera::getViewMatrix()
{
    QMatrix4x4 view; //观察矩阵
    view.lookAt(_cameraPosition,_cameraPosition+_cameraFront,_cameraUp);
    return view;
}
