#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#ifndef Q_MOC_RUN
#include <QGLWidget>
#include <QOpenGLFunctions>
#endif // Q_MOC_RUN
#include <memory>
#include "Buffer.hpp"
#include "Fractal.hpp"

class QOpenGLBuffer;
class QOpenGLShaderProgram;
class CFFmpegPlayer;

class CGLWidget: public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit CGLWidget(QWidget* parent = nullptr, QGLWidget* shareWidget = nullptr);
    ~CGLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    bool UpdateTexture(const CBuffer& buffer, GLuint&_texture,
        GLenum bufferFormat, GLint internalFormat /*, int& textureWidth, int& textureHeight*/);

    GLuint m_fractalTexture;
    GLuint m_ffmpegPlayerTexture;
    GLuint m_lookupTexture;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_vertexBuffer;

    int m_fractalLoc;
    int m_ffmpegLoc;

    std::unique_ptr<CFFmpegPlayer> m_ffmpegPlayer;
    CBuffer m_ffmpegPlayerBuf;

    CFractal m_fractal;
    CBuffer m_fractalBuf;
};

#endif // GLWIDGET_HPP
