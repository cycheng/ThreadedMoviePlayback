#include "Stdafx.hpp"
#include "Worker.hpp"
#include "TextureObject.hpp"
#include <cassert>

CWorker::CWorker() : m_pause(true), m_stop(false), m_restart(false),
m_inPauseState(false), m_inSwapWaitState(false),
m_doubleBuffer(false),
m_buffer(nullptr), m_texObj(nullptr)
{
}

CWorker::~CWorker()
{
}

void CWorker::run()
{
    {
        QMutexLocker locker(&m_mutex);
        assert(m_pause);

        m_pauseSignal.wakeOne();
        m_inPauseState = true;
        m_runSignal.wait(&m_mutex);
        m_inPauseState = false;
    }

    forever
    {
        if (!m_pause)
        {
            m_texObj->DoUpdate(m_buffer.get());
        }

        {
            QMutexLocker locker(&m_mutex);

            if (m_pause)
            {
                m_pauseSignal.wakeOne();
                m_inPauseState = true;
                m_runSignal.wait(&m_mutex);
                m_inPauseState = false;
            }

            if (m_stop)
            {
                m_pauseSignal.wakeOne();
                break;
            }

            if (m_restart)
            {
                m_buffer->SetWorkingBufferEmpty();
                m_restart = false;
                continue;
            }

            // We are ready to swap working buffer, set working buffer status
            // to full so render (paintGL()) can swap it with stable buffer
            // later.
            m_buffer->SetWorkingBufferFull();
            if (!m_buffer->CanWeSwapWorkingBuffer())
            {
                m_inSwapWaitState = true;
                m_swapBufferSignal.wait(&m_mutex);
                m_inSwapWaitState = false;
            }

            m_buffer->SwapWorkingBuffer();
        }
    }
}

const CBuffer* CWorker::GetUpdatedBufferAndSignalWorker()
{
    QMutexLocker locker(&m_mutex);

    if (m_buffer->CanWeSwapStableBuffer())
    {
        m_buffer->SwapStableBuffer();

        locker.unlock();
        m_swapBufferSignal.wakeOne();
    }

    return m_buffer.get();
}

void CWorker::Pause()
{
    QMutexLocker locker(&m_mutex);

    if (m_inPauseState)
    {
        return;
    }

    if (m_inSwapWaitState)
        m_swapBufferSignal.wakeOne();

    if (m_texObj)
        m_texObj->StopUpdate();

    m_pause = true;
    m_pauseSignal.wait(&m_mutex);
}

void CWorker::Resume(bool restartCompute)
{
    QMutexLocker locker(&m_mutex);

    // Don't resume if there is no work to do
    if (m_texObj == nullptr) {
        return;
    }

    m_restart = restartCompute;
    if (m_inPauseState)
    {
        m_pause = false;

        locker.unlock();
        m_runSignal.wakeOne();
    }
}

void CWorker::Stop()
{
    QMutexLocker locker(&m_mutex);

    if (m_stop) {
        return;
    }

    m_stop = true;

    if (m_inSwapWaitState)
    {
        m_swapBufferSignal.wakeOne();
    }

    if (m_inPauseState)
    {
        m_pause = false;
        m_runSignal.wakeOne();
    }

    m_pauseSignal.wait(&m_mutex);
}

void CWorker::UseDoubleBuffer()
{
    if (m_doubleBuffer && m_buffer)
    {
        return;
    }

    std::unique_ptr<CWorkerBuffer> old = std::move(m_buffer);
    m_buffer.reset(new CDoubleBuffer);

    if (old != false)
    {
        CloneOldBufferResultToNewBuffer(old.get(), m_buffer.get());
    }
    m_doubleBuffer = true;
}

void CWorker::UseTripleBuffer()
{
    if (!m_doubleBuffer && m_buffer)
    {
        return;
    }

    std::unique_ptr<CWorkerBuffer> old = std::move(m_buffer);
    m_buffer.reset(new CTripleBuffer);

    if (old != false)
    {
        CloneOldBufferResultToNewBuffer(old.get(), m_buffer.get());
    }
    m_doubleBuffer = false;
}

void CWorker::CloneOldBufferResultToNewBuffer(CWorkerBuffer* oldbuf, CWorkerBuffer* newbuf)
{
    newbuf->SetPixelSize(oldbuf->GetPixelSize());
    newbuf->SetTextureSize(oldbuf->GetWidth(), oldbuf->GetHeight());

    // choose an updated buffer to copy
    if (oldbuf->CanWeSwapStableBuffer()) {
        newbuf->InitAllInternalBuffers(oldbuf->GetIntermediateBuffer(),
            oldbuf->GetSize());
    }
    else {
        newbuf->InitAllInternalBuffers(oldbuf->GetStableBuffer(),
            oldbuf->GetSize());
    }
}

void CWorker::BindTextureObject(CTextureObject* texObj)
{
    m_texObj = texObj;
    m_buffer->SetPixelSize(texObj->GetBuffer()->GetPixelSize());
    texObj->m_worker = this;
}

CBuffer* CWorker::GetInternalBuffer()
{
    return m_buffer.get();
}
