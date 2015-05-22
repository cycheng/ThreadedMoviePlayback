#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#ifndef Q_MOC_RUN
#include <QGLWidget>
#include <QOpenGLFunctions>
#endif // Q_MOC_RUN
#include <memory>

class QOpenGLBuffer;
class QOpenGLShaderProgram;

class CBuffer;
class CWorker;
class CTextureObject;
class CVideoTexture;
class CFractalTexture;

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

public slots:
    void SetAnimated(int state);
    void ChangeAlphaValue(int alpha);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    void CreateTexture(CTextureObject* texObj);
    void UpdateTexture(const CTextureObject* texObj, const CBuffer* buf);

    GLuint m_lookupTexture;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_vertexBuffer;

    int m_fractalLoc;
    int m_ffmpegLoc;
    int m_alphaLoc;
    float m_alpha;

    std::unique_ptr<CVideoTexture> m_videoTex;
    std::unique_ptr<CFractalTexture> m_fractalTex;

    std::vector<CWorker*> m_threads;
    std::vector<CTextureObject*> m_textures;

    BUFFER_MODE m_bufferMode;
    bool m_threadMode;
};

#endif // GLWIDGET_HPP
