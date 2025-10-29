#include "GLFrameBufferQt.h"
#include <QPainter>

GLFrameBufferQt::GLFrameBufferQt(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus); //默认没有焦点

    QSurfaceFormat fmt = format();
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(4, 5);
    setFormat(fmt);

    //右键菜单
    menu = new QMenu(this);
    //0默认，1反相，2灰度，3锐化，4模糊，5边缘检测
    menu->addAction("Default", [this]{ algorithmType = 0; update(); });
    menu->addAction("Inversion", [this]{ algorithmType = 1; update(); });
    menu->addAction("Gray", [this]{ algorithmType = 2; update(); });
    menu->addAction("Sharpen", [this]{ algorithmType = 3; update(); });
    menu->addAction("Blur", [this]{ algorithmType = 4; update(); });
    menu->addAction("Edge-detection", [this]{ algorithmType = 5; update(); });
    menu->addAction("Set Fill Mode", [this]{ drawMode = 0; update(); });
    menu->addAction("Set Line Mode", [this]{ drawMode = 1; update(); });
    menu->addAction("Set Point Mode", [this]{ drawMode = 2; update(); });
}

GLFrameBufferQt::~GLFrameBufferQt()
{
    //initializeGL在显示时才调用，释放未初始化的会异常
    if(!isValid())
        return;
    //QOpenGLWidget
    //三个虚函数不需要makeCurrent，对应的操作已由框架完成
    //但是释放时需要设置当前上下文
    makeCurrent();
    freeScreen();
    freeFbo();
    doneCurrent();
}

void GLFrameBufferQt::initializeGL()
{
    //QOpenGLFunctions
    //为当前上下文初始化opengl函数解析
    initializeOpenGLFunctions();

    initScreen();
    initFbo();
}

void GLFrameBufferQt::paintGL()
{
    //渲染自定义帧缓冲
    paintFbo();
    //渲染默认帧缓冲
    paintScreen();
}

void GLFrameBufferQt::resizeGL(int width, int height)
{
    if (width < 1 || height < 1) {
        return;
    }
    //重置自定义帧缓冲大小
    resetFbo();
}

void GLFrameBufferQt::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    if(event->button()==Qt::RightButton){
        menu->popup(QCursor::pos());
    }else{
        mousePos = event->pos();
    }
}

void GLFrameBufferQt::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void GLFrameBufferQt::mouseMoveEvent(QMouseEvent *event)
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

void GLFrameBufferQt::wheelEvent(QWheelEvent *event)
{
    event->accept();
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    //const QPoint pos = event->pos();
    const int delta = event->delta();
#else
    //const QPoint pos = event->position().toPoint();
    const int delta = event->angleDelta().y();
#endif
    //fovy越小，模型看起来越大
    if(delta < 0){
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

void GLFrameBufferQt::initScreen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str = R"(#version 450
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0f, 1.0f);
})";
    //算法选择：0默认，1反相，2灰度，3锐化，4模糊，5边缘检测
    const char *fragment_str = R"(#version 450
out vec4 FragColor;
in vec2 TexCoords;
uniform int algorithm;
const float offset = 1.0f / 300.0f;
uniform sampler2D screenTexture;
void main()
{
vec2 offsets[9] = vec2[](
    vec2(-offset,  offset),
    vec2( 0.0f,    offset),
    vec2( offset,  offset),
    vec2(-offset,  0.0f),
    vec2( 0.0f,    0.0f),
    vec2( offset,  0.0f),
    vec2(-offset, -offset),
    vec2( 0.0f,   -offset),
    vec2( offset, -offset)
);
switch(algorithm)
{
case 0:{
vec3 color = texture(screenTexture, TexCoords).rgb;
FragColor = vec4(color, 1.0f);
}break;
case 1:{
vec3 color = texture(screenTexture, TexCoords).rgb;
FragColor = vec4(1.0f - color, 1.0f);
}break;
case 2:{
vec3 color = texture(screenTexture, TexCoords).rgb;
float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
FragColor = vec4(average, average, average, 1.0f);
}break;
case 3:{
float kernels[9] = float[](
-1, -1, -1,
-1,  9, -1,
-1, -1, -1
);
vec3 sampleTex[9];
for(int i = 0; i < 9; ++i)
    sampleTex[i] = texture(screenTexture, TexCoords.st + offsets[i]).rgb;
vec3 color;
for(int i = 0; i < 9; ++i)
    color += sampleTex[i] * kernels[i];
FragColor = vec4(color, 1.0f);
}break;
case 4:{
float kernels[9] = float[](
1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
);
vec3 sampleTex[9];
for(int i = 0; i < 9; ++i)
    sampleTex[i] = texture(screenTexture, TexCoords.st + offsets[i]).rgb;
vec3 color;
for(int i = 0; i < 9; ++i)
    color += sampleTex[i] * kernels[i];
FragColor = vec4(color, 1.0f);
}break;
case 5:{
float kernels[9] = float[](
1,  1,  1,
1, -9,  1,
1,  1,  1
);
vec3 sampleTex[9];
for(int i = 0; i < 9; ++i)
    sampleTex[i] = texture(screenTexture, TexCoords.st + offsets[i]).rgb;
vec3 color;
for(int i = 0; i < 9; ++i)
    color += sampleTex[i] * kernels[i];
FragColor = vec4(color, 1.0f);
}break;
default:{
vec3 color = texture(screenTexture, TexCoords).rgb;
FragColor = vec4(color, 1.0f);
}break;
}
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!screenShaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<screenShaderProgram.log();
    }
    if(!screenShaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<screenShaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!screenShaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<screenShaderProgram.log();
    }

    //顶点数据
    GLfloat quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };

    screenVao.create();
    screenVao.bind();
    screenVbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    screenVbo.create();
    screenVbo.bind();
    screenVbo.allocate(quadVertices, sizeof(quadVertices));
    screenShaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2, sizeof(GLfloat) * 4);
    screenShaderProgram.enableAttributeArray(0);
    screenShaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 2, 2, sizeof(GLfloat) * 4);
    screenShaderProgram.enableAttributeArray(1);

    screenShaderProgram.bind();
    screenShaderProgram.setUniformValue("screenTexture", 0);
    screenShaderProgram.release();
}

void GLFrameBufferQt::initFbo()
{
    resetFbo();
    glBindFramebuffer(GL_FRAMEBUFFER, fboBuffer->handle());

    //着色器代码
    //in输入，out输出,uniform从cpu向gpu发送
    const char *vertex_str = R"(#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
})";
    const char *fragment_str = R"(#version 450
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D texture1;
void main()
{
    FragColor = texture(texture1, TexCoords);
})";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!fboShaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,vertex_str)){
        qDebug()<<"compiler vertex error"<<fboShaderProgram.log();
    }
    if(!fboShaderProgram.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,fragment_str)){
        qDebug()<<"compiler fragment error"<<fboShaderProgram.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!fboShaderProgram.link()){
        qDebug()<<"link shaderprogram error"<<fboShaderProgram.log();
    }

    //顶点数据，一个立方体cube和一个平面plane
    GLfloat cubeVertices[] = {
        // positions          // texture Coords
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
    GLfloat planeVertices[] = {
        // positions          // texture Coords
        5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

        5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
        5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };

    //cube
    fboCubeVao.create();
    fboCubeVao.bind();
    fboCubeVbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    fboCubeVbo.create();
    fboCubeVbo.bind();
    fboCubeVbo.allocate(cubeVertices, sizeof(cubeVertices));
    fboShaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    fboShaderProgram.enableAttributeArray(0);
    fboShaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    fboShaderProgram.enableAttributeArray(1);

    //plane
    fboPlaneVao.create();
    fboPlaneVao.bind();
    fboPlaneVbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    fboPlaneVbo.create();
    fboPlaneVbo.bind();
    fboPlaneVbo.allocate(planeVertices, sizeof(planeVertices));
    fboShaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 5);
    fboShaderProgram.enableAttributeArray(0);
    fboShaderProgram.setAttributeBuffer(1, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(GLfloat) * 5);
    fboShaderProgram.enableAttributeArray(1);

    fboShaderProgram.bind();
    fboShaderProgram.setUniformValue("texture1", 0);
    fboShaderProgram.release();

    //2d纹理
    fboCubeTexture = new QOpenGLTexture(QImage(":/container.jpg").mirrored(), QOpenGLTexture::GenerateMipMaps);
    if(!fboCubeTexture->isCreated()){
        qDebug() << "Failed to load texture";
    }
    fboCubeTexture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    fboCubeTexture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    fboCubeTexture->setMinificationFilter(QOpenGLTexture::Linear);
    fboCubeTexture->setMagnificationFilter(QOpenGLTexture::Linear);

    fboPlaneTexture = new QOpenGLTexture(QImage(":/metal.png").mirrored(), QOpenGLTexture::GenerateMipMaps);
    if(!fboPlaneTexture->isCreated()){
        qDebug() << "Failed to load texture";
    }
    fboPlaneTexture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    fboPlaneTexture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);//
    fboPlaneTexture->setMinificationFilter(QOpenGLTexture::Linear);
    fboPlaneTexture->setMagnificationFilter(QOpenGLTexture::Linear);
}

void GLFrameBufferQt::resetFbo()
{
    if(fboBuffer){
        delete fboBuffer;
    }

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setMipmap(true); //默认为false
    //format.setSamples(8);
    format.setTextureTarget(GL_TEXTURE_2D); //默认为GL_TEXTURE_2D
    format.setInternalTextureFormat(GL_RGBA);

    const QSize device_size = size() * devicePixelRatioF();
    fboBuffer = new QOpenGLFramebufferObject(device_size, format);
    fboBuffer->bind();
    glBindTexture(GL_TEXTURE_2D, fboBuffer->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fboBuffer->addColorAttachment(fboBuffer->size(), GL_RGBA);
    fboBuffer->release();
}

void GLFrameBufferQt::paintScreen()
{
    if(!fboBuffer){
        return;
    }
    //切换到QOpenGLWidget.context默认的帧缓冲，这里不能用0来作为默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //绘制模式
    if(drawMode == 0){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }else if(drawMode == 1){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(4.0f);
    }

    screenShaderProgram.bind();
    screenVao.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboBuffer->texture());
    screenShaderProgram.setUniformValue("algorithm", algorithmType);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    screenShaderProgram.release();
    //恢复
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 14));
    painter.drawText(20, 40, "Click right mouse button popup menu");
}

void GLFrameBufferQt::paintFbo()
{
    if(!fboBuffer){
        return;
    }
    //渲染自定义帧缓冲
    fboBuffer->bind();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //激活纹理
    glActiveTexture(GL_TEXTURE0);

    fboShaderProgram.bind();
    //透视投影
    QMatrix4x4 projection;
    projection.perspective(projectionFovy, 1.0f * width() / height(), 0.1f, 100.0f);
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -5.0f);
    view.rotate(rotationQuat);
    QMatrix4x4 model;
    fboShaderProgram.setUniformValue("view", view);
    fboShaderProgram.setUniformValue("projection", projection);

    fboCubeTexture->bind();
    fboCubeVao.bind();
    model.setToIdentity();
    model.translate(-1.0f, 0.0f, -1.0f);
    fboShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    model.setToIdentity();
    model.translate(2.0f, 0.0f, 0.0f);
    fboShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    fboPlaneTexture->bind();
    fboPlaneVao.bind();
    model.setToIdentity();
    model.translate(0.0f, -0.01f, 0.0f);
    fboShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    fboShaderProgram.release();
    fboBuffer->release();

    //渲染 FBO 后恢复一些全局 GL 状态，防止影响后续默认帧缓冲的绘制
    //例如深度测试在 FBO 绘制时被启用，若不禁用可能会导致后续绘制（如全屏四边形）被测试丢弃
    glDisable(GL_DEPTH_TEST);
}

void GLFrameBufferQt::freeScreen()
{
    screenVbo.destroy();
    screenVao.destroy();
}

void GLFrameBufferQt::freeFbo()
{
    fboCubeVbo.destroy();
    fboCubeVao.destroy();
    if(fboCubeTexture){
        delete fboCubeTexture;
    }
    fboPlaneVbo.destroy();
    fboPlaneVao.destroy();
    if(fboPlaneTexture){
        delete fboPlaneTexture;
    }
    if(fboBuffer){
        delete fboBuffer;
    }
}
