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
    explicit CGLWidget(QWidget* parent = nullptr, QGLWidget* shareWidget = nullptr);
    ~CGLWidget();

    void ChangeBufferMode(BUFFER_MODE mode);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    void CreateTexture(CTextureObject* texObj);
    void UpdateTexture(const CTextureObject* texObj, const CBuffer* buf);
    void ProcessEventAfterPaint();

    GLuint m_fractalTexture;
    GLuint m_ffmpegPlayerTexture;
    GLuint m_lookupTexture;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_vertexBuffer;

    int m_fractalLoc;
    int m_ffmpegLoc;

    std::unique_ptr<CVideoTexture> m_videoTex;
    std::unique_ptr<CFractalTexture> m_fractalTex;

    std::vector<CWorker*> m_threads;
    std::vector<CTextureObject*> m_textures;

    BUFFER_MODE m_bufferMode;
    bool m_threadMode;
    bool m_fireBufferModeChange;
};

/*
 *  usage:
 *      Pause(), then you can call Resume() or Stop()
 */
class CWorker : public QThread
{
public:
    CWorker();

    void run() override;
    void Pause();
    void Stop();
    void Resume();

    const CBuffer* GetUpdatedBufferAndSignalWorker();

    //void UseDoubleBuffer();
    void UseTripleBuffer();
    void BindTextureObject(CTextureObject* texObj);
    CBuffer* GetInternalBuffer();

private:
    QMutex m_mutex;

    QWaitCondition m_runSignal;
    QWaitCondition m_pauseSignal;
    QWaitCondition m_swapBufferSignal;

    bool m_pause;
    bool m_stop;
    bool m_inPauseState;
    bool m_inSwapWaitState;

    std::unique_ptr<CWorkerBuffer> m_buffer;
    CTextureObject* m_texObj;
};

class CTextureObject
{
public:
    CTextureObject();
    virtual ~CTextureObject();

    virtual void DoUpdate(CBuffer* buffer) = 0;
    virtual void Resize(int width, int height);

    void Update();
    void BindWorker(CWorker* worker);
    void SetTextureFormat(GLenum bufferFmt, GLint internalFmt);
    void SetNewTextureId(GLuint newGLId);

    CBuffer* GetBuffer();
    CWorker* GetWorker();

    GLuint GetTextureID() const;
    GLenum GetBufferFormat() const;
    GLint GetInternalFormat() const;

private:
    CWorker* m_worker;
    CSingleBuffer m_buffer;
    GLuint m_textureId;
    GLenum m_bufferFmt;
    GLint m_internalFmt;
};

class CVideoTexture: public CTextureObject
{
public:
    void DoUpdate(CBuffer* buffer) override;
    void Resize(int width, int height) override;

    bool ChangeVideo(const std::string& fileName);

private:
    std::unique_ptr<CFFmpegPlayer> m_ffmpegPlayer;
};

class CFractalTexture: public CTextureObject
{
public:
    CFractalTexture();
    void DoUpdate(CBuffer* buffer) override;

private:
    std::unique_ptr<CFractal> m_fractal;
};

#endif // GLWIDGET_HPP
