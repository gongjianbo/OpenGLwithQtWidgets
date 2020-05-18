#include "GLColors.h"

#include <QDebug>

GLColors::GLColors(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus); //Widget默认没有焦点
}

GLColors::~GLColors()
{
    //通过使相应的上下文成为当前上下文并在该上下文中绑定帧缓冲区对象，
    //准备为此小部件呈现OpenGL内容。在调用paintGL()之前会自动调用。
    makeCurrent();
    _vbo.destroy();
    _lightingVao.destroy();
    _lampVao.destroy();
    //释放上下文。在大多数情况下不需要调用此函数，
    //因为小部件将确保在调用paintGL()时正确绑定和释放上下文。
    doneCurrent();
}

void GLColors::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();
    initShader();

    //VAO,VBO
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
    };

    _vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    _vbo.create();

    //light vao
    _lightingVao.create();
    _lightingVao.bind();
    _vbo.bind();
    _vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    int attr = -1;
    attr = _lightingShader.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _lightingShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    _lightingShader.enableAttributeArray(attr);
    _vbo.release();
    _lightingVao.release();

    //lamp vao
    _lampVao.create();
    _lampVao.bind();
    _vbo.bind();
    attr = _lampShader.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _lampShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 3);
    _lampShader.enableAttributeArray(attr);
    _vbo.release();
    _lampVao.release();
}

void GLColors::paintGL()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    //draw lighting
    _lightingShader.bind();
    _lightingShader.setUniformValue("objectColor",QVector3D(1.0f,0.5f,0.31f));
    _lightingShader.setUniformValue("lightColor",QVector3D(1.0f,1.0f,1.0f));
    QMatrix4x4 view=_camera.getViewMatrix(); //观察矩阵
    //QMatrix4x4 view;
    //view.translate(QVector3D(0.0f, 0.0f, -10.0f));
    //qDebug()<<"view"<<view;
    _lightingShader.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(_camera.getZoom(), 1.0f * width() / height(), 0.1f, 100.0f);
    _lightingShader.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    model.rotate(45,QVector3D(1.0f,1.0f,0.0f));
    _lightingShader.setUniformValue("model", model);
    _lightingVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    _lightingVao.release();
    _lightingShader.release();

    //draw lamp
    _lampShader.bind();
    _lampShader.setUniformValue("view", view);
    _lampShader.setUniformValue("projection", projection);
    model=QMatrix4x4();
    model.translate(QVector3D(1.2f, 1.0f, 2.0f));
    model.scale(0.2f);
    _lampShader.setUniformValue("model", model);
    _lampVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    _lampVao.release();
    _lampShader.release();
}

void GLColors::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLColors::keyPressEvent(QKeyEvent *event)
{
    _camera.keyPress(event->key());
    update();
    QOpenGLWidget::keyPressEvent(event);
}

void GLColors::mousePressEvent(QMouseEvent *event)
{
    _camera.mousePress(event->pos());
    update();
    QOpenGLWidget::mousePressEvent(event);
}

void GLColors::mouseReleaseEvent(QMouseEvent *event)
{
    _camera.mouseRelease(event->pos());
    update();
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLColors::mouseMoveEvent(QMouseEvent *event)
{
    _camera.mouseMove(event->pos());
    update();
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLColors::wheelEvent(QWheelEvent *event)
{
    _camera.mouseWheel(event->delta());
    update();
    QOpenGLWidget::wheelEvent(event);
}

void GLColors::initShader()
{
    //lingting shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *lighting_vertex=R"(#version 330 core
                                layout (location = 0) in vec3 aPos;
                                uniform mat4 model;
                                uniform mat4 view;
                                uniform mat4 projection;

                                void main()
                                {
                                gl_Position = projection * view * model * vec4(aPos, 1.0f);
                                })";
    const char *lighting_fragment=R"(#version 330 core
                                  uniform vec3 objectColor;
                                  uniform vec3 lightColor;
                                  out vec4 FragColor;

                                  void main()
                                  {
                                  FragColor = vec4(lightColor * objectColor, 1.0);
                                  })";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!_lightingShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,lighting_vertex)){
        qDebug()<<"compiler vertex error"<<_lightingShader.log();
    }
    if(!_lightingShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,lighting_fragment)){
        qDebug()<<"compiler fragment error"<<_lightingShader.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!_lightingShader.link()){
        qDebug()<<"link shaderprogram error"<<_lightingShader.log();
    }

    //lamp shader
    const char *lamp_vertex=R"(#version 330 core
                            layout (location = 0) in vec3 aPos;
                            uniform mat4 model;
                            uniform mat4 view;
                            uniform mat4 projection;

                            void main()
                            {
                            gl_Position = projection * view * model * vec4(aPos, 1.0f);
                            })";
    const char *lamp_fragment=R"(#version 330 core
                              out vec4 FragColor;
                              void main()
                              {
                              FragColor = vec4(1.0);
                              })"; // set alle 4 vector values to 1.0

    if(!_lampShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,lamp_vertex)){
        qDebug()<<"compiler vertex error"<<_lampShader.log();
    }
    if(!_lampShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,lamp_fragment)){
        qDebug()<<"compiler fragment error"<<_lampShader.log();
    }
    if(!_lampShader.link()){
        qDebug()<<"link shaderprogram error"<<_lampShader.log();
    }
}
