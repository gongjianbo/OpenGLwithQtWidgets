#include "GLLightingMapsB.h"

#include <QDebug>

GLLightingMapsB::GLLightingMapsB(QWidget *parent)
    : QOpenGLWidget(parent)
{
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,this,[this](){
        _theAngle=(_theAngle+5)%360;
        update();
    });
    _timer->start(150);
}

GLLightingMapsB::~GLLightingMapsB()
{
    //通过使相应的上下文成为当前上下文并在该上下文中绑定帧缓冲区对象，
    //准备为此小部件呈现OpenGL内容。在调用paintGL()之前会自动调用。
    makeCurrent();
    _vbo.destroy();
    _cubeVao.destroy();
    _lampVao.destroy();
    delete _textureDiffuse;
    delete _textureSpecular;
    //释放上下文。在大多数情况下不需要调用此函数，
    //因为小部件将确保在调用paintGL()时正确绑定和释放上下文。
    doneCurrent();
}

void GLLightingMapsB::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();
    initShader();

    //VAO,VBO
    //position 顶点，normal 法向量，texture 纹理坐标
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    _vbo=QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    _vbo.create();

    //cube vao
    _cubeVao.create();
    _cubeVao.bind();
    _vbo.bind();
    _vbo.allocate(vertices,sizeof(vertices));
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    int attr = -1;
    attr = _cubeShader.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _cubeShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    _cubeShader.enableAttributeArray(attr);
    attr = _cubeShader.attributeLocation("aNormal");
    _cubeShader.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8);
    _cubeShader.enableAttributeArray(attr);
    attr = _cubeShader.attributeLocation("aTexCoord");
    _cubeShader.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8);
    _cubeShader.enableAttributeArray(attr);
    _vbo.release();
    _cubeVao.release();

    //漫反射纹理
    //绑定一个2d纹理, 并生成多级纹理MipMaps
    _textureDiffuse=new QOpenGLTexture(QImage(":/box2.png"),QOpenGLTexture::GenerateMipMaps);
    if(!_textureDiffuse->isCreated()){
        qDebug() << "Failed to load texture";
    }
    //set the texture wrapping parameters
    //等价于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _textureDiffuse->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    _textureDiffuse->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    // set texture filtering parameters
    //等价于glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _textureDiffuse->setMinificationFilter(QOpenGLTexture::Linear);
    _textureDiffuse->setMagnificationFilter(QOpenGLTexture::Linear);

    //镜面光纹理
    //box2border.png和box3rain.jpg两张图不同效果
    //_textureSpecular=new QOpenGLTexture(QImage(":/box2border.png"),QOpenGLTexture::GenerateMipMaps);
    _textureSpecular=new QOpenGLTexture(QImage(":/box2rain.jpg"),QOpenGLTexture::GenerateMipMaps);
    if(!_textureSpecular->isCreated()){
        qDebug() << "Failed to load texture";
    }
    _textureSpecular->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
    _textureSpecular->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
    _textureSpecular->setMinificationFilter(QOpenGLTexture::Linear);
    _textureSpecular->setMagnificationFilter(QOpenGLTexture::Linear);

    _cubeShader.bind();
    _cubeShader.setUniformValue("material.diffuse", 0);
    _cubeShader.setUniformValue("material.specular", 1);
    _cubeShader.release();

    //lamp vao
    _lampVao.create();
    _lampVao.bind();
    _vbo.bind();
    attr = _lampShader.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _lampShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8);
    _lampShader.enableAttributeArray(attr);
    _vbo.release();
    _lampVao.release();
}

void GLLightingMapsB::paintGL()
{
    //清除后默认颜色
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

    //纹理单元
    glActiveTexture(GL_TEXTURE0);
    _textureDiffuse->bind();
    glActiveTexture(GL_TEXTURE1);
    _textureSpecular->bind();
    //懒得调camera了，写个固定的矩阵来观测就行了
    //draw cube
    _cubeShader.bind();
    _cubeShader.setUniformValue("viewPos",QVector3D(0.0f,0.0f,3.0f));
    //ambient光照下反射的颜色
    //diffuse漫反射
    //specular镜面反射
    //shininess反光度
    //光源对环境光、漫反射和镜面光分量也具有着不同的强度
    QVector3D light_pos(1.2f, 1.0f, 2.0f);
    QVector3D light_color(1.0f,1.0f,1.0f);
    QVector3D ambient_color=light_color*0.5f;
    QVector3D diffuse_color=ambient_color*0.2f;
    _cubeShader.setUniformValue("light.position",light_pos);
    _cubeShader.setUniformValue("light.ambient",ambient_color);
    _cubeShader.setUniformValue("light.diffuse",diffuse_color);
    _cubeShader.setUniformValue("light.specular",QVector3D(1.0f, 1.0f, 1.0f));
    //材质
    //_cubeShader.setUniformValue("material.ambient",QVector3D(1.0f, 0.5f, 0.31f));
    //_cubeShader.setUniformValue("material.diffuse",QVector3D(1.0f, 0.5f, 0.31f));
    //_cubeShader.setUniformValue("material.specular",QVector3D(0.5f, 0.5f, 0.5f));
    _cubeShader.setUniformValue("material.shininess",64.0f);

    QMatrix4x4 view;//观察矩阵
    view.translate(QVector3D(-0.5f, -0.3f, -4.0f));
    _cubeShader.setUniformValue("view", view);
    QMatrix4x4 projection; //透视投影
    projection.perspective(45.0f, 1.0f * width() / height(), 0.1f, 100.0f);
    _cubeShader.setUniformValue("projection", projection);
    QMatrix4x4 model;//模型矩阵
    model.rotate(-20,QVector3D(1.0f,0.0f,0.0f));
    model.rotate(_theAngle,QVector3D(0.0f,1.0f,0.0f));
    _cubeShader.setUniformValue("model", model);
    _cubeVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    _cubeVao.release();
    _cubeShader.release();

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

void GLLightingMapsB::initShader()
{
    //cube shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *cube_vertex=R"(#version 330 core
                            layout (location = 0) in vec3 aPos;
                            layout (location = 1) in vec3 aNormal;
                            layout (location = 2) in vec2 aTexCoord;

                            out vec3 Pos;
                            out vec3 Normal;
                            out vec2 TexCoord;

                            uniform mat4 model;
                            uniform mat4 view;
                            uniform mat4 projection;

                            void main()
                            {
                            Pos = vec3(model * vec4(aPos, 1.0));
                            Normal = mat3(transpose(inverse(model))) * aNormal;
                            TexCoord = aTexCoord;

                            gl_Position = projection * view * vec4(Pos, 1.0);
                            })";
    //用纹理sampler2D替代了之前的ambient和diffuse材质分量
    const char *cube_fragment=R"(#version 330 core
                              out vec4 FragColor;

                              struct Material {
                              sampler2D diffuse;
                              sampler2D specular;
                              float shininess;
                              };

                              struct Light {
                              vec3 position;

                              vec3 ambient;
                              vec3 diffuse;
                              vec3 specular;
                              };

                              in vec3 Pos;
                              in vec3 Normal;
                              in vec2 TexCoord;

                              uniform vec3 viewPos;
                              uniform Material material;
                              uniform Light light;

                              void main()
                              {
                              // ambient
                              vec3 ambient = light.ambient * texture(material.diffuse, TexCoord).rgb;

                              // diffuse
                              vec3 norm = normalize(Normal);
                              vec3 lightDir = normalize(light.position - Pos);
                              float diff = max(dot(norm, lightDir), 0.0);
                              vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoord).rgb;

                              // specular
                              vec3 viewDir = normalize(viewPos - Pos);
                              vec3 reflectDir = reflect(-lightDir, norm);
                              float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
                              vec3 specular = light.specular * spec * texture(material.specular, TexCoord).rgb;

                              vec3 result = ambient + diffuse + specular;
                              FragColor = vec4(result, 1.0);
                              }  )";

    //将source编译为指定类型的着色器，并添加到此着色器程序
    if(!_cubeShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Vertex,cube_vertex)){
        qDebug()<<"compiler vertex error"<<_cubeShader.log();
    }
    if(!_cubeShader.addCacheableShaderFromSourceCode(
                QOpenGLShader::Fragment,cube_fragment)){
        qDebug()<<"compiler fragment error"<<_cubeShader.log();
    }
    //使用addShader()将添加到该程序的着色器链接在一起。
    if(!_cubeShader.link()){
        qDebug()<<"link shaderprogram error"<<_cubeShader.log();
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
                              })";

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
