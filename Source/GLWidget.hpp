#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#ifndef Q_MOC_RUN
#include <QGLWidget>
#include <QOpenGLFunctions>
#endif // Q_MOC_RUN
#include <memory>

#include "TextureObject.hpp"
#include "Effect.hpp"

class QOpenGLBuffer;
class QOpenGLShaderProgram;

class CGLWidget: public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum BUFFER_MODE {
        BF_SINGLE = 1, BF_TRIPLE
    };

    class PauseWorkers
    {
    public:
        explicit PauseWorkers(CGLWidget* widget);
        ~PauseWorkers();
    private:
        CGLWidget* m_widget;
    };
    friend PauseWorkers;

public:
    explicit CGLWidget(QWidget* parent = nullptr, QGLWidget* shareWidget = nullptr);
    ~CGLWidget();

    void ChangeBufferMode(BUFFER_MODE mode);
    static QOpenGLFunctions* m_glProvider;

public slots:
    void SetAnimated(int state);
    void ChangeAlphaValue(int alpha);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    GLuint m_lookupTexture;
    QOpenGLBuffer* m_vertexBuffer;

    CVideoTexture m_videoTex;
    CFractalTexture m_fractalTex;

    CFractalFX m_fractalfx;
    CFluidFX m_fluidfx;

    std::vector<CEffect*> m_effects;
    std::vector<CWorker*> m_threads;
    std::vector<CTextureObject*> m_threadTextures;

    BUFFER_MODE m_bufferMode;
    bool m_threadMode;
};

#endif // GLWIDGET_HPP
