#include "OpenGLWidget.h"
#include <QDebug>


OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_texture(nullptr), m_imgData(nullptr), m_imgWidth(0), m_imgHeight(0) {}

void OpenGLWidget::setImageData(uint8_t *data, int w, int h)
{
    m_imgData = data;

    //切换视频时适应大小
    if(m_imgWidth != w || m_imgHeight != h)
    {
        m_imgWidth = w;
        m_imgHeight = h;
        resizeGL(width(), height());
    }

    // 如果纹理已经存在，释放旧纹理
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }

    update();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // OpenGL信息
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    qDebug() << "Renderer:" << renderer;
    qDebug() << "Verdor:" << vendor;
    qDebug() << "Qt OpenGL Type:" << (QOpenGLContext::openGLModuleType() ? "OpenGL Desktop" : "OpenGL ES");

    // 设置清除颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // 创建着色器程序
    m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    R"(
                                    #version 330 core
                                    in vec2 position;
                                    in vec2 texCoord;
                                    out vec2 TexCoord;
                                    void main() {
                                        gl_Position = vec4(position, 0.0, 1.0);
                                        TexCoord = texCoord;
                                    }
                                    )");
    m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    R"(
                                    #version 330 core
                                    in vec2 TexCoord;
                                    out vec4 color;
                                    uniform sampler2D ourTexture;
                                    void main() {
                                        color = texture(ourTexture, TexCoord);
                                    }
                                    )");
    m_shader.link();

    //创建 VAO 和 VBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    updateVertices(1.0f, 1.0f);
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // 如果没有图像数据，不进行渲染
    if (!m_imgData || m_imgWidth == 0 || m_imgHeight == 0) return;

    // 更新纹理
    if (!m_texture) {
        m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_texture->setSize(m_imgWidth, m_imgHeight);
        m_texture->allocateStorage();
    }

    m_texture->bind();
    m_texture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, m_imgData);

    // 使用着色器和 VAO 渲染
    m_shader.bind();
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    m_texture->release();
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    if(!m_imgWidth || !m_imgHeight)
        return;

    // 计算当前视口和图像的比例，调整显示区域
    float aspectRatio = static_cast<float>(m_imgWidth) / static_cast<float>(m_imgHeight);
    float windowAspectRatio = static_cast<float>(w) / static_cast<float>(h);

    if (aspectRatio > windowAspectRatio)
        updateVertices(1.0f,  windowAspectRatio / aspectRatio);  // 如果图像宽高比大于窗口宽高比，缩放高度
    else
        updateVertices(aspectRatio / windowAspectRatio, 1.0f);   // 如果图像宽高比小于或等于窗口宽高比，缩放宽度
}

void OpenGLWidget::updateVertices(float scaleX, float scaleY)
{
    // 顶点数据和纹理坐标，根据缩放因子调整
    GLfloat vertices[] = {
        // 定点三维坐标      // 纹理坐标（翻转 y 轴）
        -scaleX,  scaleY, 0.0f, 0.0f,  // 左上角
        -scaleX, -scaleY, 0.0f, 1.0f,  // 左下角
         scaleX,  scaleY, 1.0f, 0.0f,   // 右上角
         scaleX, -scaleY, 1.0f, 1.0f,   // 右下角
    };

    // 更新 VBO 和 VAO
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // 设置顶点属性指针
    m_shader.bind();
    GLuint posLocation = m_shader.attributeLocation("position");
    glVertexAttribPointer(posLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(posLocation);

    GLuint texLocation = m_shader.attributeLocation("texCoord");
    glVertexAttribPointer(texLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(texLocation);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
