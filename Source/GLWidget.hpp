#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#ifndef Q_MOC_RUN
#include <QGLWidget>
#include <QOpenGLFunctions>
#endif // Q_MOC_RUN

class QOpenGLBuffer;
class QOpenGLShaderProgram;

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
	bool UpdateTexture(/* buffer, */ GLuint&_texture, int& textureWidth, int& textureHeight);

    GLuint m_fractalTexture;
    GLuint m_ffmpegPlayerTexture;
    GLuint m_lookupTexture;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_vertexBuffer;
};

#endif // GLWIDGET_HPP
