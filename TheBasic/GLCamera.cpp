#include "GLCamera.h"
#include <cmath>
#include <QtMath>

GLCamera::GLCamera(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&timer,&QTimer::timeout,this,[this](){
        rotate+=2;
        if(isVisible()){
            update();
        }
    });
    timer.setInterval(50);

    setFocusPolicy(Qt::ClickFocus); //默认没有焦点
    calculateCamera();
}

GLCamera::~GLCamera()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    vbo.destroy();
    vao.destroy();
    delete texture1;
    delete texture2;
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
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 texCoord;
void main()
{
gl_Position = projection * view * model * vec4(inPos, 1.0);
texCoord = vec2(inTexCoord.x, 1-inTexCoord.y);
})";
    const char *fragment_str=R"(#version 330 core
in vec2 texCoord;
uniform sampler2D texture1;
uniform sampler2D texture2;
out vec4 fragColor;
void main()
{
fragColor = mix(texture(texture1, texCoord), texture(texture2, texCoord), 0.2);
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<shaderProgram.log();
    }
    if(!shaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<shaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!shaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<shaderProgram.log();
    }

    //顶点数据（盒子六个面，一个面两个三角）
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
    vao.create();
    vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));

    //参数1对应layout
    // position attribute
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    shaderProgram.enableAttributeArray(0);
    // texture coord attribute
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    shaderProgram.enableAttributeArray(1);

    // texture 1
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture1 = new QOpenGLTexture(QImage(":/container.jpg"), QOpenGLTexture::GenerateMipMaps);
    if(!texture1->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture1->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture1->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    // set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture1->setMinificationFilter(QOpenGLTexture::Linear);
    texture1->setMagnificationFilter(QOpenGLTexture::Linear);

    // texture 2
    //直接生成绑定一个2d纹理, 并生成多级纹理MipMaps
    texture2 = new QOpenGLTexture(QImage(":/awesomeface.png"), QOpenGLTexture::GenerateMipMaps);
    if(!texture2->isCreated()){
        qDebug() << "Failed to load texture";
    }
    // set the texture wrapping parameters
    // 等于glTexParameteri(GLtexture_2D, GLtexture_WRAP_S, GL_REPEAT);
    texture2->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    texture2->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    // set texture filtering parameters
    //等价于glTexParameteri(GLtexture_2D, GLtexture_MIN_FILTER, GL_LINEAR);
    texture2->setMinificationFilter(QOpenGLTexture::Linear);
    texture2->setMagnificationFilter(QOpenGLTexture::Linear);

    shaderProgram.bind();
    shaderProgram.setUniformValue("texture1", 0);
    shaderProgram.setUniformValue("texture2", 1);
    shaderProgram.release();

    timer.start();
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
    //因为我们使用了深度测试，需要在每次渲染迭代之前清除深度缓冲
    //（否则前一帧的深度信息仍然保存在缓冲中）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer)，也被称为深度缓冲(Depth Buffer)
    //（不开启深度缓冲的话，盒子的纹理堆叠顺序就是乱的）
    glEnable(GL_DEPTH_TEST); //默认关闭的

    //纹理
    glActiveTexture(GL_TEXTURE0);
    texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    texture2->bind();

    shaderProgram.bind();
    vao.bind();

    /*//观察矩阵
    QMatrix4x4 view;
    //3个相互垂直的轴和一个定义摄像机空间的位置坐标
    //void QMatrix4x4::lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up)
    //第一组参数：eyex, eyey,eyez 相机在世界坐标的位置
    //第二组参数：centerx,centery,centerz 相机镜头对准的物体在世界坐标的位置
    //第三组参数：upx,upy,upz 相机向上的方向在世界坐标中的方向，(0,-1,0)就旋转了190度
    view.lookAt(cameraPos,
                cameraPos+cameraFront,
                cameraUp);
    shaderProgram.setUniformValue("view", view);*/
    shaderProgram.setUniformValue("view", getViewMatrix());
    //透视投影
    QMatrix4x4 projection;
    //void QMatrix4x4::perspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane)
    //参数1视野，即观察空间大小
    //参数2宽高比
    //参数3平截头体近距离，参数4平截头体远距离
    //近平面和远平面内且处于平截头体内的顶点都会被渲染
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    shaderProgram.setUniformValue("projection", projection);
    for (unsigned int i = 0; i < 10; i++) {
        //计算模型矩阵
        QMatrix4x4 model;
        //平移
        model.translate(cubePositions[i]);
        //这样每个箱子旋转的速度就不一样
        float angle = (i + 1.0f) * rotate;
        //旋转
        model.rotate(angle, QVector3D(1.0f, 0.3f, 0.5f));
        //传入着色器并绘制
        shaderProgram.setUniformValue("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    vao.release();
    texture1->release();
    texture2->release();
    shaderProgram.release();
}

void GLCamera::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLCamera::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    //横向是移动，不是绕0点
    switch (event->key()) {
    case Qt::Key_W: //摄像机往上，场景往下
        cameraPos -= QVector3D::crossProduct(cameraFront, cameraRight).normalized() * cameraSpeed;
        break;
    case Qt::Key_S: //摄像机往下，场景往上
        cameraPos += QVector3D::crossProduct(cameraFront, cameraRight).normalized() * cameraSpeed;
        break;
    case Qt::Key_A: //摄像机往左，场景往右
        cameraPos -= QVector3D::crossProduct(cameraFront, cameraUp).normalized() * cameraSpeed;
        break;
    case Qt::Key_D: //摄像机往右，场景往左
        cameraPos += QVector3D::crossProduct(cameraFront, cameraUp).normalized() * cameraSpeed;
        break;
    case Qt::Key_E: //远
        cameraPos -= cameraFront * cameraSpeed;
        break;
    case Qt::Key_Q: //近
        cameraPos += cameraFront * cameraSpeed;
        break;
    default:
        break;
    }
    update();
}

void GLCamera::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
}

void GLCamera::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void GLCamera::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    int x_offset = event->pos().x()-mousePos.x();
    int y_offset = event->pos().y()-mousePos.y();
    mousePos = event->pos();
    //y轴的坐标是从下往上，所以相反
    //我这里移动的是摄像机，所以场景往相反方向动
    eulerYaw += x_offset*cameraSensitivity;
    eulerPitch -= y_offset*cameraSensitivity;

    if (eulerPitch > 89.0f)
        eulerPitch = 89.0f;
    else if (eulerPitch < -89.0f)
        eulerPitch = -89.0f;
    calculateCamera();
    update();
}

void GLCamera::wheelEvent(QWheelEvent *event)
{
    event->accept();
    //fovy越小，模型看起来越大
    if(event->delta() < 0){
        //鼠标向下滑动为-，这里作为zoom out
        projectionFovy += cameraSpeed;
        if(projectionFovy > 90)
            projectionFovy = 90;
    }else{
        //鼠标向上滑动为+，这里作为zoom in
        projectionFovy -= cameraSpeed;
        if(projectionFovy < 1)
            projectionFovy = 1;
    }
    update();
}

void GLCamera::calculateCamera()
{
    //角度转弧度
    //2021-8-29 之前没转为弧度制，导致初始位置不对
    const float yaw = qDegreesToRadians(eulerYaw);
    const float pitch = qDegreesToRadians(eulerPitch);
    QVector3D front;
    front.setX(std::cos(yaw) * std::cos(pitch));
    front.setY(std::sin(pitch));
    front.setZ(std::sin(yaw) * std::cos(pitch));
    cameraFront = front.normalized();
}

QMatrix4x4 GLCamera::getViewMatrix()
{
    QMatrix4x4 view; //观察矩阵
    view.lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
    return view;
}
