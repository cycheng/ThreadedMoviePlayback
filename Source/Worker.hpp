#ifndef WORKER_HPP
#define WORKER_HPP

#include <memory>
#include <QThread>
#include <QWaitCondition>

// ----------------------------------------------------------------------------
// Worker thread for texture object update
// ----------------------------------------------------------------------------
class CBuffer;
class CWorkerBuffer;
class CTextureObject;

class CWorker : public QThread
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

    void UseDoubleBuffer();
    void UseTripleBuffer();
    void BindTextureObject(CTextureObject* texObj);
    CBuffer* GetInternalBuffer();

private:
    void CloneOldBufferResultToNewBuffer(CWorkerBuffer* oldbuf, CWorkerBuffer* newbuf);

    QMutex m_mutex;
    QWaitCondition m_runSignal;
    QWaitCondition m_pauseSignal;
    QWaitCondition m_swapBufferSignal;

    bool m_pause;
    bool m_stop;
    bool m_restart;
    bool m_inPauseState;
    bool m_inSwapWaitState;
    bool m_doubleBuffer;

    std::unique_ptr<CWorkerBuffer> m_buffer;
    CTextureObject* m_texObj;
};

#endif  // WORKER_HPP
