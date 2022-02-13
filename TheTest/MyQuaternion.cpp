#include "MyQuaternion.h"
#include <cmath>
#include <QtMath>

//着色器代码
static const char *coord_vertex=R"(#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 theColor;
void main()
{
theColor = inColor;
gl_Position = projection * view * model * vec4(inPos, 1.0);
})";

static const char *coord_fragment=R"(#version 330 core
in vec3 theColor;
out vec4 fragColor;
void main()
{
fragColor = vec4(theColor, 1.0);
})";

static const char *box_vertex=R"(#version 330 core
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

static const char *box_fragment=R"(#version 330 core
in vec2 texCoord;
uniform sampler2D texture1;
uniform sampler2D texture2;
out vec4 fragColor;
void main()
{
fragColor = mix(texture(texture1, texCoord), texture(texture2, texCoord), 0.2);
})";

//坐标系
static float coord_vertices[] = {
    0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    5.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    0.0f,  0.0f,  0.0f,  0.5f,  0.0f,  0.0f,
    -5.0f, 0.0f,  0.0f,  0.5f,  0.0f,  0.0f,

    0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  5.0f,  0.0f,  0.0f,  1.0f,  0.0f,

    0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.0f,
    0.0f,  -5.0f, 0.0f,  0.0f,  0.5f,  0.0f,

    0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  5.0f,  0.0f,  0.0f,  1.0f,

    0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.5f,
    0.0f,  0.0f,  -5.0f, 0.0f,  0.0f,  0.5f
};

//顶点数据（盒子六个面，一个面两个三角）
static float box_vertices[] = {
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

//绘制多个盒子
static QVector3D cubePositions[] = {
    QVector3D( 0.0f,  0.0f,  0.0f),
    QVector3D( 0.0f,  4.0f,  0.0f),//y轴对齐
    QVector3D( 4.0f,  0.0f,  0.0f),//x轴对齐
    QVector3D( 2.0f,  4.0f, -9.0f),
    QVector3D(-1.5f, -2.2f, -2.5f),
    QVector3D(-3.8f, -2.0f, -7.3f),
    QVector3D( 2.4f, -0.4f, -3.5f),
    QVector3D(-1.7f,  3.0f, -7.5f),
    QVector3D( 5.3f, -2.0f, -2.5f),
    QVector3D(-1.3f,  1.0f, -1.5f)
};

MyQuaternion::MyQuaternion(QWidget *parent)
    : QOpenGLWidget(parent)
{
    connect(&timer,&QTimer::timeout,this,[this](){
        boxRotate+=2;
        if(isVisible()){
            update();
        }
    });
    timer.setInterval(50);

    setFocusPolicy(Qt::ClickFocus); //默认没有焦点

    rotationQuat = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 30.0f);
    rotationQuat *= QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -10.0f);
}

MyQuaternion::~MyQuaternion()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    coordVbo.destroy();
    coordVao.destroy();
    boxVbo.destroy();
    boxVao.destroy();
    delete texture1;
    delete texture2;
    doneCurrent();
}

void MyQuaternion::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //【】坐标系
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!coordProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,coord_vertex)){
        qDebug()<<"compiler vertex error"<<coordProgram.log();
    }
    if(!coordProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,coord_fragment)){
        qDebug()<<"compiler fragment error"<<coordProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!coordProgram.link()){
        qDebug()<<"link shaderprogram error"<<coordProgram.log();
    }
    coordVao.create();
    coordVao.bind();
    coordVbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    coordVbo.create();
    coordVbo.bind();
    coordVbo.allocate(coord_vertices,sizeof(coord_vertices));

    //参数1对应layout
    // position attribute
    coordProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    coordProgram.enableAttributeArray(0);
    // color attribute
    coordProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    coordProgram.enableAttributeArray(1);

    //【】盒子
    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!boxProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,box_vertex)){
        qDebug()<<"compiler vertex error"<<boxProgram.log();
    }
    if(!boxProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,box_fragment)){
        qDebug()<<"compiler fragment error"<<boxProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!boxProgram.link()){
        qDebug()<<"link shaderprogram error"<<boxProgram.log();
    }

    boxVao.create();
    boxVao.bind();
    boxVbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    boxVbo.create();
    boxVbo.bind();
    boxVbo.allocate(box_vertices,sizeof(box_vertices));

    //参数1对应layout
    // position attribute
    boxProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    boxProgram.enableAttributeArray(0);
    // texture coord attribute
    boxProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    boxProgram.enableAttributeArray(1);

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

    boxProgram.bind();
    boxProgram.setUniformValue("texture1", 0);
    boxProgram.setUniformValue("texture2", 1);
    boxProgram.release();

    timer.start();
}

void MyQuaternion::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //因为我们使用了深度测试，需要在每次渲染迭代之前清除深度缓冲
    //（否则前一帧的深度信息仍然保存在缓冲中）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer)，也被称为深度缓冲(Depth Buffer)
    //（不开启深度缓冲的话，盒子的纹理堆叠顺序就是乱的）
    glEnable(GL_DEPTH_TEST); //默认关闭的

    //观察矩阵
    QMatrix4x4 view;
    float radius = 20.0f;
    //float camX = std::sin(rotate*0.02) * radius;
    //float camZ = std::cos(rotate*0.02) * radius;
    //view.lookAt(QVector3D(camX, 2.0f, camZ),
    //            QVector3D(0.0f, 0.0f, 0.0f),
    //            QVector3D(0.0f, 1.0f, 0.0f));
    view.translate(0.0f, 0.0f, -radius);
    //QQuaternion rotate_x = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, xRotate);
    //QQuaternion rotate_y = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, yRotate);
    view.rotate(rotationQuat);
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    coordProgram.bind();
    coordVao.bind();
    coordProgram.setUniformValue("view", view);
    coordProgram.setUniformValue("projection", projection);
    coordProgram.setUniformValue("model", QMatrix4x4());
    glDrawArrays(GL_LINES, 0, 12);
    coordVao.release();
    coordProgram.release();

    //纹理
    glActiveTexture(GL_TEXTURE0);
    texture1->bind();
    glActiveTexture(GL_TEXTURE1);
    texture2->bind();

    boxProgram.bind();
    boxVao.bind();
    boxProgram.setUniformValue("view", view);
    boxProgram.setUniformValue("projection", projection);
    for (unsigned int i = 0; i < 10; i++) {
        //模型矩阵
        QMatrix4x4 model;
        //平移
        model.translate(cubePositions[i]);
        //这样每个箱子旋转的速度就不一样
        float angle = (i + 1.0f) * boxRotate;
        //旋转
        model.rotate(angle, QVector3D(1.0f, 0.3f, 0.5f));
        //传入着色器并绘制
        boxProgram.setUniformValue("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    boxVao.release();
    texture1->release();
    texture2->release();
    boxProgram.release();
}

void MyQuaternion::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void MyQuaternion::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    mousePos = event->pos();
}

void MyQuaternion::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void MyQuaternion::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    //参照示例cube
    QVector2D diff = QVector2D(event->pos()) - QVector2D(mousePos);
    mousePos = event->pos();
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    rotationAxis = (rotationAxis + n).normalized();
    //不能对换乘的顺序
    rotationQuat = QQuaternion::fromAxisAndAngle(rotationAxis, 2.0f) * rotationQuat;

    update();
}

void MyQuaternion::wheelEvent(QWheelEvent *event)
{
    event->accept();
    //fovy越小，模型看起来越大
    if(event->delta() < 0){
        //鼠标向下滑动为-，这里作为zoom out
        projectionFovy += 0.5f;
        if(projectionFovy > 90)
            projectionFovy = 90;
    }else{
        //鼠标向上滑动为+，这里作为zoom in
        projectionFovy -= 0.5f;
        if(projectionFovy < 1)
            projectionFovy = 1;
    }
    update();
}
