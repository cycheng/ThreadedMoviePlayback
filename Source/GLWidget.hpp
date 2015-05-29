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
    enum BUFFER_MODE
    {
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

    enum EFFECT
    {
        FX_BASE = 0,
        FX_FRACTAL,
        FX_FLUID,
        FX_TOTAL
    };

public:
    explicit CGLWidget(QWidget* parent = nullptr, QGLWidget* shareWidget = nullptr);
    ~CGLWidget();

    void ChangeBufferMode(BUFFER_MODE mode);
    static QOpenGLFunctions* m_glProvider;

public slots:
    void SetAnimated(int state);
    void ChangeAlphaValue(int alpha);
    void EnableFX(EFFECT id);
    void DisableFX(EFFECT id);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    void CreateTextureRenderTarget(int width, int height);
    void DestroyTextureRenderTarget();

    GLuint m_lookupTexture;
    QOpenGLBuffer* m_vertexBuffer;

    CVideoTexture m_videoTex;
    CFractalTexture m_fractalTex;

    CMoviePlayback m_basefx;
    CFractalFX m_fractalfx;
    CFluidFX m_fluidfx;

    CEffect* m_effects[FX_TOTAL];
    std::vector<CWorker*> m_threads;
    std::vector<CTextureObject*> m_threadTextures;

    BUFFER_MODE m_bufferMode;
    bool m_threadMode;

    // For render to texture, shared in all Effects
    GLuint m_fboId;
    GLuint m_fboTextureId;
    GLuint m_renderBufferId;
};

#endif // GLWIDGET_HPP
