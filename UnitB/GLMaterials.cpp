#include "GLMaterials.h"

#include <QDebug>

GLMaterials::GLMaterials(QWidget *parent)
    : QOpenGLWidget(parent)
{
    static bool angle_flag=true;
    static bool shin_flag=true;
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,this,[this](){
        if(_theAngle>30){
            angle_flag=false;
        }else if(_theAngle<-10){
            angle_flag=true;
        }
        if(angle_flag){
            _theAngle+=1.0f;
        }else{
            _theAngle-=1.0f  ;
        }

        if(_theShininess>64){
            shin_flag=false;
        }else if(_theShininess<8){
            shin_flag=true;
        }
        if(shin_flag){
           _theShininess+=0.5f;
        }else{
           _theShininess-=1.0f;
        }
        update();
    });
    _timer->start(150);
}

GLMaterials::~GLMaterials()
{
    //通过使相应的上下文成为当前上下文并在该上下文中绑定帧缓冲区对象，
    //准备为此小部件呈现OpenGL内容。在调用paintGL()之前会自动调用。
    makeCurrent();
    _vbo.destroy();
    _cubeVao.destroy();
    _lampVao.destroy();
    //释放上下文。在大多数情况下不需要调用此函数，
    //因为小部件将确保在调用paintGL()时正确绑定和释放上下文。
    doneCurrent();
}

void GLMaterials::initializeGL()
{
    //为当前上下文初始化OpenGL函数解析
    initializeOpenGLFunctions();
    initShader();

    //VAO,VBO
    //为什么没用EBO，是复制粘贴的啊，懒得改
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
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
    _cubeShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    _cubeShader.enableAttributeArray(attr);
    attr = _cubeShader.attributeLocation("aNormal");
    _cubeShader.setAttributeBuffer(attr, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 6);
    _cubeShader.enableAttributeArray(attr);
    _vbo.release();
    _cubeVao.release();

    //lamp vao
    _lampVao.create();
    _lampVao.bind();
    _vbo.bind();
    attr = _lampShader.attributeLocation("aPos");
    //setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0)
    _lampShader.setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(GLfloat) * 6);
    _lampShader.enableAttributeArray(attr);
    _vbo.release();
    _lampVao.release();
}

void GLMaterials::paintGL()
{
    //清除后默认颜色
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    //清除深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //Z缓冲(Z-buffer),深度缓冲(Depth Buffer)。
    glEnable(GL_DEPTH_TEST);

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
    _cubeShader.setUniformValue("material.ambient",QVector3D(1.0f, 0.5f, 0.31f));
    _cubeShader.setUniformValue("material.diffuse",QVector3D(1.0f, 0.5f, 0.31f));
    _cubeShader.setUniformValue("material.specular",QVector3D(0.5f, 0.5f, 0.5f));
    _cubeShader.setUniformValue("material.shininess",_theShininess);

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

void GLMaterials::initShader()
{
    //cube shader
    //in输入，out输出,uniform从cpu向gpu发送
    const char *cube_vertex=R"(#version 330 core
                            layout (location = 0) in vec3 aPos;
                            layout (location = 1) in vec3 aNormal;
                            out vec3 Pos;
                            out vec3 Normal;

                            uniform mat4 model;
                            uniform mat4 view;
                            uniform mat4 projection;

                            void main()
                            {
                            Pos = vec3(model * vec4(aPos, 1.0));
                            Normal = mat3(transpose(inverse(model))) * aNormal;
                            gl_Position = projection * view * vec4(Pos, 1.0);
                            })";
    const char *cube_fragment=R"(#version 330 core
                              out vec4 FragColor;

                              struct Material {
                              vec3 ambient;
                              vec3 diffuse;
                              vec3 specular;
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

                              uniform vec3 viewPos;
                              uniform Material material;
                              uniform Light light;

                              void main()
                              {
                              // ambient
                              vec3 ambient = light.ambient * material.ambient;

                              // diffuse
                              vec3 norm = normalize(Normal);
                              vec3 lightDir = normalize(light.position - Pos);
                              float diff = max(dot(norm, lightDir), 0.0);
                              vec3 diffuse = light.diffuse * (diff * material.diffuse);

                              // specular
                              vec3 viewDir = normalize(viewPos - Pos);
                              vec3 reflectDir = reflect(-lightDir, norm);
                              float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
                              vec3 specular = light.specular * (spec * material.specular);

                              vec3 result = ambient + diffuse + specular;
                              FragColor = vec4(result, 1.0);
                              })";

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
