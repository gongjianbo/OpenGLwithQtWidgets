#include "MyCoordinate.h"
#include <QPushButton>
#include <QWidget>
#include <QGridLayout>
#include <QDebug>

MyCoordinate::MyCoordinate(QWidget *parent)
    : QOpenGLWidget(parent)
{
    QPushButton *top=new QPushButton("top",this);
    top->setFixedSize(90,30);
    top->move(100,10);
    connect(top,&QPushButton::clicked,[this]{
        modelMat.rotate(10, QVector3D(1.0f, 0.0f, 0.0f));
        update();
    });
    QPushButton *bottom=new QPushButton("bottom",this);
    bottom->setFixedSize(90,30);
    bottom->move(100,70);
    connect(bottom,&QPushButton::clicked,[this]{
        modelMat.rotate(-10, QVector3D(1.0f, 0.0f, 0.0f));
        update();
    });
    QPushButton *left=new QPushButton("left",this);
    left->setFixedSize(90,30);
    left->move(10,40);
    connect(left,&QPushButton::clicked,[this]{
        modelMat.rotate(10, QVector3D(0.0f, 1.0f, 0.0f));
        update();
    });
    QPushButton *right=new QPushButton("right",this);
    right->setFixedSize(90,30);
    right->move(190,40);
    connect(right,&QPushButton::clicked,[this]{
        modelMat.rotate(-10, QVector3D(0.0f, 1.0f, 0.0f));
        update();
    });
    QPushButton *reset=new QPushButton("reset",this);
    reset->setFixedSize(90,30);
    reset->move(100,40);
    connect(reset,&QPushButton::clicked,[this]{
        modelMat.setToIdentity();
        //modelMat.translate(QVector3D(0.0f,  0.0f,  0.0f));
        update();
    });
}

MyCoordinate::~MyCoordinate()
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
    doneCurrent();
}

void MyCoordinate::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    //因为OpenGL纹理颠倒过来的，所以取反vec2(aTexCoord.x, 1-aTexCoord.y);
    const char *vertex_str=R"(#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 theColor;
void main()
{
gl_Position = projection * view * model * vec4(inPos, 1.0);
theColor = inColor;
})";
    const char *fragment_str=R"(#version 330 core
in vec3 theColor;
out vec4 fragColor;
void main()
{
fragColor = vec4(theColor, 1.0);
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
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, //back-white
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f,

        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, //face-red
        0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
        0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
        0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, //left-green
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,

        0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, //right-blue
        0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
        0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, //bottom-cyan
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,

        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f, //top-yellow
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
        0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
        0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f
    };
    vao.create();
    vao.bind();
    //QOpenGLVertexArrayObject::Binder vaoBind(&vao);
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));

    //参数1对应layout
    //顶点
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    shaderProgram.enableAttributeArray(0);
    //颜色
    shaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    shaderProgram.enableAttributeArray(1);

    shaderProgram.bind();
    QMatrix4x4 view; //观察矩阵，后退一点
    //OpenGL本身没有摄像机(Camera)的概念，但我们可以通过把场景中的所有物体往相反方向移动的方式来模拟出摄像机，
    //产生一种我们在移动的感觉，而不是场景在移动。
    view.translate(QVector3D(0.0f, 0.0f, -3.0f));
    shaderProgram.setUniformValue("view", view);
    //透视投影现在放到paintGL
    shaderProgram.release();
}

void MyCoordinate::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //因为我们使用了深度测试，需要在每次渲染迭代之前清除深度缓冲
    //（否则前一帧的深度信息仍然保存在缓冲中）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //OpenGL存储它的所有深度信息于一个Z缓冲(Z-buffer)中，也被称为深度缓冲(Depth Buffer)。
    //深度值存储在每个片段里面（作为片段的z值），当片段想要输出它的颜色时，OpenGL会将它的深度值和z缓冲进行比较，
    //如果当前的片段在其它片段之后，它将会被丢弃，否则将会覆盖。
    //这个过程称为深度测试(Depth Testing)，它是由OpenGL自动完成的。
    //（不开启深度缓冲的话，盒子的纹理堆叠顺序就是乱的）
    glEnable(GL_DEPTH_TEST); //默认关闭的

    shaderProgram.bind();
    vao.bind();

    QMatrix4x4 projection; //透视投影
    //坐标到达观察空间之后，我们需要将其投影到裁剪坐标。
    //裁剪坐标会被处理至-1.0到1.0的范围内，并判断哪些顶点将会出现在屏幕上
    //参数1：指定视景体的视野的角度
    //参数2：指定你的视景体的宽高比
    //参数3：指定观察者到视景体的最近的裁剪面的距离（正数）
    //参数4：指定观察者到视景体最远的裁剪面距离（正数）
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    shaderProgram.setUniformValue("projection", projection);

    //计算模型矩阵
    //QMatrix4x4 model;
    //平移
    //model.translate(QVector3D( 0.0f,  0.0f,  0.0f));
    //旋转
    //model.rotate(30, QVector3D(0.0f, 0.0f, 1.0f));
    //传入着色器并绘制
    shaderProgram.setUniformValue("model", modelMat);

    vao.release();
    shaderProgram.release();
}

void MyCoordinate::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}
