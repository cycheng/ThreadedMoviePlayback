#ifndef TEXTUREOBJECT_HPP
#define TEXTUREOBJECT_HPP

#include "Buffer.hpp"

#include <QThread>
#include <QWaitCondition>
#include <QOpenGLFunctions>

class CFFmpegPlayer;
class CFractal;
class CTextureObject;

class CWorker: public QThread
{
public:
    CWorker();

    void run() override;
    // You can call Resume() or Stop() after Pause()
    void Pause();
    void Stop();
    void Resume(bool restartCompute);

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
    bool m_restart;
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
    virtual void StopUpdate();

    void Update();
    void BindWorker(CWorker* worker);
    void SetTextureFormat(GLenum bufferFmt, GLint internalFmt);
    void SetNewTextureId(GLuint newGLId);

    CBuffer* GetBuffer();
    CWorker* GetWorker();

    GLuint GetTextureID() const;
    GLenum GetBufferFormat() const;
    GLint GetInternalFormat() const;

protected:
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

#include "Fractal.hpp"
class CFractalTexture: public CTextureObject, public CFractal
{
public:
    CFractalTexture();
    void DoUpdate(CBuffer* buffer) override;
    void StopUpdate() override;
};

QOpenGLFunctions& GL();

#endif  // TEXTUREOBJECT_HPP
