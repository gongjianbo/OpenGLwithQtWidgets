#include "MyTorus.h"
#include <QtMath>
#include <QPainter>
#include <QCursor>

MyTorus::MyTorus(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus); //默认没有焦点
    rotationQuat = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90.0f);

    //设置成core核心模式QPainter才能正常的绘制
    QSurfaceFormat fmt = format();
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(4, 5);
    setFormat(fmt);

    //右键菜单
    menu = new QMenu(this);
    menu->addAction("Toggle depth test", [this]{
        enableDepthTest = !enableDepthTest;
        update();
    });
    menu->addAction("Toggle cull backface", [this]{
        enableCullBackFace = !enableCullBackFace;
        update();
    });
    menu->addAction("Set Fill Mode", [this]{
        drawMode = 0;
        update();
    });
    menu->addAction("Set Line Mode", [this]{
        drawMode = 1;
        update();
    });
    menu->addAction("Set Point Mode", [this]{
        drawMode = 2;
        update();
    });
}

MyTorus::~MyTorus()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    doneCurrent();
}

void MyTorus::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str=R"(#version 450 core
layout (location = 0) in vec3 vPos;
uniform mat4 mvp;
out vec3 thePos;
void main() {
gl_Position = mvp * vec4(vPos, 1.0f);
thePos = vPos;
})";
    const char *fragment_str=R"(#version 450 core
in vec3 thePos;
out vec4 fragColor;
void main() {
float red = abs(sqrt(thePos.x * thePos.x + thePos.z * thePos.z));
fragColor = vec4(red, 0.0f, 0.0f, 1.0f);
})";

    //顶点着色器
    //可以直接add着色器代码，也可以借助QOpenGLShader类
    bool success=shaderProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,vertex_str);
    if(!success){
        qDebug()<<"compiler vertex failed!"<<shaderProgram.log();
    }
    //片段着色器
    success=shaderProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,fragment_str);
    if(!success){
        qDebug()<<"compiler fragment failed!"<<shaderProgram.log();
    }
    success = shaderProgram.link();
    if(!success){
        qDebug()<<"link shader failed!"<<shaderProgram.log();
    }

    float a,x,y;
    const float out_r=1.0f;//外圆半径
    const float in_r=0.5f;//内圆半径
    const float section_r=(out_r-in_r)/2;//截面圆环半径
    const int section_slice=24;//截面圆边定点数
    const int torus_slice=24;//整体圆环截面数
    //1.先求一个截面的顶点做基准
    QVector<QVector3D> section_vec;
    for(int i=0;i<section_slice;i++)
    {
        a=qDegreesToRadians(360.0f/section_slice*i);
        x=section_r*cos(a);
        y=section_r*sin(a);
        QVector3D vec(x,y,0);
        section_vec.push_back(vec);
    }
    //2.再对截面定点进行坐标变换，形成一个环面的顶点
    //3.对顶点进行组织，使之能围成三角
    QMatrix4x4 cur_mat;//当前截面的变换矩阵
    QMatrix4x4 next_mat;//下一个截面的变换矩阵
    next_mat.setToIdentity();
    next_mat.translate(out_r-section_r,0,0);
    for(int i=0;i<torus_slice;i++)
    {
        cur_mat=next_mat;
        next_mat.setToIdentity();
        next_mat.rotate(360.0f/torus_slice*(i+1),0,1.0f,0);
        next_mat.translate(out_r-section_r,0,0);
        for(int j=0;j<section_vec.size();j++)
        {
            vertex.push_back(cur_mat * section_vec.at(j));
            vertex.push_back(next_mat * section_vec.at(j));
        }
        vertex.push_back(vertex.at(vertex.size()-section_vec.size()*2));
        vertex.push_back(vertex.at(vertex.size()-section_vec.size()*2));
    }

    vao.create();
    vao.bind();
    vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate((void *)vertex.data(), sizeof(GLfloat) * vertex.size() * 3);
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    shaderProgram.enableAttributeArray(0);
    vbo.release();
    vao.release();

    //清屏设置
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void MyTorus::paintGL()
{
    //深度测试
    if(enableDepthTest){
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }else{
        glClear(GL_COLOR_BUFFER_BIT);
    }
    //面剔除
    if(enableCullBackFace){
        glEnable(GL_CULL_FACE);
    }else{
        glDisable(GL_CULL_FACE);
    }
    //绘制模式
    if(drawMode==0){
        //正反面都设置为填充
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else if(drawMode==1){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(4.0f);
    }

    shaderProgram.bind();
    //观察矩阵
    QMatrix4x4 view;
    float radius = 3.0f;
    view.translate(0.0f, 0.0f, -radius);
    view.rotate(rotationQuat);
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    shaderProgram.setUniformValue("mvp", projection * view);
    if(!vertex.isEmpty())
    {
        QOpenGLVertexArrayObject::Binder vao_bind(&vao); Q_UNUSED(vao_bind);
        //使用当前激活的着色器和顶点属性配置和VBO（通过VAO间接绑定）来绘制图元
        //void glDrawArrays(GLenum mode​, GLint first​, GLsizei count​);
        //参数1为图元类型
        //参数2指定顶点数组的起始索引
        //参数3指定顶点个数
        //GL_TRIANGLE_STRIP三角形带
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex.size());
    }
    shaderProgram.release();

    //设置为fill，不然会影响QPainter绘制的图
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei",16));
    painter.drawText(20, 50, "Click right mouse button popup menu");
}

void MyTorus::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void MyTorus::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    if(event->button()==Qt::RightButton){
        menu->popup(QCursor::pos());
    }else{
        mousePos = event->pos();
    }
}

void MyTorus::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void MyTorus::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    //参照示例cube
    QVector2D diff = QVector2D(event->pos()) - QVector2D(mousePos);
    mousePos = event->pos();
    QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();
    rotationAxis = (rotationAxis + n).normalized();
    //不能对换乘的顺序
    rotationQuat = QQuaternion::fromAxisAndAngle(rotationAxis, 3.0f) * rotationQuat;

    update();
}

void MyTorus::wheelEvent(QWheelEvent *event)
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
