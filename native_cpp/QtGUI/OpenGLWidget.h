#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <iostream>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    OpenGLWidget(QWidget* parent = nullptr);

    // 设置图像数据
    void setImageData(uint8_t* data, int w, int h);

protected:
    void initializeGL() override;

    void paintGL() override;

    void resizeGL(int w, int h) override;

private:
    void updateVertices(float scaleX, float scaleY);

    QOpenGLShaderProgram m_shader;
    QOpenGLTexture* m_texture;
    GLuint m_VAO, m_VBO;
    uint8_t* m_imgData;
    int m_imgWidth;
    int m_imgHeight;
};



#endif // OPENGLWIDGET_H
