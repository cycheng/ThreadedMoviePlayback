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

typedef std::unique_ptr<CBuffer> u_buffer_ptr;
typedef std::unique_ptr<CFractal> u_fractal_ptr;
typedef std::unique_ptr<CFFmpegPlayer> u_ffmpegplayer_ptr;


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
    bool UpdateTexture(CBuffer* buffer, GLuint&_texture,
        GLenum bufferFormat, GLint internalFormat /*, int& textureWidth, int& textureHeight*/);

    GLuint m_fractalTexture;
    GLuint m_ffmpegPlayerTexture;
    GLuint m_lookupTexture;
    QOpenGLShaderProgram* m_program;
    QOpenGLBuffer* m_vertexBuffer;

    int m_fractalLoc;
    int m_ffmpegLoc;

    u_ffmpegplayer_ptr m_ffmpegPlayer;
    u_buffer_ptr m_ffmpegPlayerBuf;

    u_fractal_ptr m_fractal;
    u_buffer_ptr m_fractalBuf;

    std::vector<CWorker*> m_threads;
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
protected:
    CThreadBuffer* m_buffer;
private:
    QMutex m_mutex;
    QWaitCondition m_runSignal;
    QWaitCondition m_pauseSignal;
    bool m_pause;
    bool m_stop;
    bool m_inPauseState;

    virtual void DoCompute() = 0;
};

class CVideoWorker : public CWorker
{
public:
    CVideoWorker(CFFmpegPlayer* playerPtr, CThreadBuffer* bufPtr);
private:
    void DoCompute() override;
    CFFmpegPlayer* m_ffmpegPlayer;
    CThreadBuffer* m_ffmpegPlayerBuf;
};

class CFractalWorker : public CWorker
{
public:
    CFractalWorker(CFractal* fractalPtr, CThreadBuffer* bufPtr);
private:
    void DoCompute() override;
    CFractal* m_fractal;
    CThreadBuffer* m_fractalBuf;
};

#endif // GLWIDGET_HPP
