#ifndef TEXTUREOBJECT_HPP
#define TEXTUREOBJECT_HPP

#include "Buffer.hpp"

#include <QThread>
#include <QWaitCondition>
#include <QOpenGLFunctions>

class CWorker;

class CTextureObject
{
public:
    CTextureObject();
    virtual ~CTextureObject();
    virtual bool Resize(int width, int height);
    virtual void StopUpdate();

    void UpdateByWorker(int elapsedMs);
    void UpdateByMySelf(int elapsedMs, bool forceUpdate = false);
    void CopyMyDataToWorker();
    void CopyWorkerDataToMe();

    void Enable();
    void Disable();

    void SetTextureFormat(GLenum bufferFmt, GLint internalFmt);
    CBuffer* GetBuffer();
    CWorker* GetWorker();
    GLuint GetTextureID() const;

protected:
    bool Timeout(int elapsedMs);
    virtual void DoUpdate(CBuffer* buffer) = 0;
    void CreateTexture();
    void UpdateTexture(const CBuffer* buf);

    CWorker* m_worker;
    CSingleBuffer m_buffer;
    GLuint m_textureId;
    GLenum m_bufferFmt;
    GLint m_internalFmt;
    quint8 m_enableCount;

    // framerate control:
    float m_time;
    float m_msPerFrame;  // ms per frame: update a frame every m_msPerFrame ms

    friend class CWorker;
};

// ----------------------------------------------------------------------------
// Worker thread for texture object update
// ----------------------------------------------------------------------------
class CWorker: public QThread
{
public:
    CWorker();
    virtual ~CWorker();

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

// ----------------------------------------------------------------------------
// Derived Texture Objects
// ----------------------------------------------------------------------------

class CFFmpegPlayer;

class CVideoTexture: public CTextureObject
{
public:
    void DoUpdate(CBuffer* buffer) override;
    bool Resize(int width, int height) override;
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
