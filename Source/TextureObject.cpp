#include "Stdafx.hpp"
#include "TextureObject.hpp"
#include "FFmpegPlayer.hpp"
#include "Fractal.hpp"

#include <cassert>
#include <iostream>

CWorker::CWorker(): m_pause(true), m_stop(false), m_restart(false),
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
        if (! m_pause)
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

CTextureObject::CTextureObject(): m_worker(nullptr), m_textureId(0),
                                  m_bufferFmt(0), m_internalFmt(0),
                                  m_enableCount(0),
                                  m_time(0), m_msPerFrame(0)
{
}

CTextureObject::~CTextureObject()
{
    GL().glDeleteTextures(1, &m_textureId);
}

bool CTextureObject::Resize(int width, int height)
{
    if (m_buffer.GetWidth() == width && m_buffer.GetHeight() == height)
    {
        return false;
    }

    if (m_worker)
    {
        CBuffer* buf = m_worker->GetInternalBuffer();
        buf->SetTextureSize(width, height);
        buf->InitIntermediateBufferWithZero();
    }
    m_buffer.SetTextureSize(width, height);
    m_buffer.InitIntermediateBufferWithZero();
    CreateTexture();

    return true;
}

void CTextureObject::StopUpdate()
{
}

void CTextureObject::UpdateByWorker(int elapsedMs)
{
    if (! m_enableCount || ! Timeout(elapsedMs))
    {
        return;
    }

    assert(m_worker && "Internal Error! This texture object should bind a worker.");

    const CBuffer* updatedBuf = m_worker->GetUpdatedBufferAndSignalWorker();
    UpdateTexture(updatedBuf);
}

void CTextureObject::UpdateByMySelf(int elapsedMs, bool forceUpdate)
{
    if (! forceUpdate && (!m_enableCount || !Timeout(elapsedMs)))
    {
        return;
    }

    if (forceUpdate)
        m_time = 0.f;

    DoUpdate(&m_buffer);
    UpdateTexture(&m_buffer);
}

void CTextureObject::CopyMyDataToWorker()
{
    assert(m_worker && "Internal Error! This texture object should bind a worker.");

    const CBuffer* src = &m_buffer;
    CBuffer* dest = m_worker->GetInternalBuffer();
    dest->InitIntermediateBuffer(src->GetIntermediateBuffer(), src->GetSize());
}

void CTextureObject::CopyWorkerDataToMe()
{
    assert(m_worker && "Internal Error! This texture object should bind a worker.");

    const CBuffer* src = m_worker->GetInternalBuffer();
    CBuffer* dest = &m_buffer;
    dest->InitIntermediateBuffer(src->GetIntermediateBuffer(), src->GetSize());
}

void CTextureObject::Enable()
{
    m_enableCount++;
}

void CTextureObject::Disable()
{
    assert(m_enableCount > 0 && "Internal Error! Incorrect enable count.");
    m_enableCount--;
}

void CTextureObject::SetTextureFormat(GLenum bufferFmt, GLint internalFmt)
{
    m_bufferFmt = bufferFmt;
    m_internalFmt = internalFmt;
    m_buffer.SetPixelSize(GetGLPixelSize(bufferFmt));

    if (m_worker)
    {
        m_worker->GetInternalBuffer()->SetPixelSize(GetGLPixelSize(bufferFmt));
    }
}

CBuffer* CTextureObject::GetBuffer()
{
    return &m_buffer;
}

CWorker* CTextureObject::GetWorker()
{
    return m_worker;
}

GLuint CTextureObject::GetTextureID() const
{
    return m_textureId;
}

bool CTextureObject::Timeout(int elapsedMs)
{
    float elapsed = (float)elapsedMs;

    if (elapsed > m_msPerFrame)
    {
        m_time = 0.f;
        return true;
    }

    m_time += elapsed;
    if (m_time > m_msPerFrame)
    {
        m_time -= m_msPerFrame;
        return true;
    }
    return false;
}

void CTextureObject::CreateTexture()
{
    if (m_textureId != 0)
    {
        glDeleteTextures(1, &m_textureId);
    }

    GL().glGenTextures(1, &m_textureId);
    GL().glBindTexture(GL_TEXTURE_2D, m_textureId);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    GL().glTexImage2D(GL_TEXTURE_2D, 0, m_internalFmt,
                      m_buffer.GetWidth(), m_buffer.GetHeight(),
                      0, m_bufferFmt, GL_UNSIGNED_BYTE, nullptr);
}

void CTextureObject::UpdateTexture(const CBuffer* buf)
{
    void* data = buf->GetStableBuffer();

    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf->GetWidth(), buf->GetHeight(),
                    m_bufferFmt, GL_UNSIGNED_BYTE, data);
}

void CVideoTexture::DoUpdate(CBuffer* buffer)
{
    if (m_ffmpegPlayer == nullptr)
    {
        memset(buffer->GetWorkingBuffer(), 0, buffer->GetSize());
        return;
    }

    unsigned int pts;
    bool newframe = false;

    while (! newframe)
    {
        newframe = m_ffmpegPlayer->decodeFrame(pts,
                                               buffer->GetWorkingBuffer(),
                                               buffer->GetRowSize());
    }
}

bool CVideoTexture::Resize(int width, int height)
{
    if (! CTextureObject::Resize(width, height))
    {
        return false;
    }

    if (m_ffmpegPlayer != nullptr)
    {
        m_ffmpegPlayer->setOutputSize(width, height);
    }

    // decode one frame to initialize result buffer
    UpdateByMySelf(0, true);
    if (m_worker)
    {
        m_worker->GetInternalBuffer()->InitIntermediateBuffer(
            m_buffer.GetWorkingBuffer(), m_buffer.GetSize());
    }
    return true;
}

bool CVideoTexture::ChangeVideo(const std::string& fileName)
{
    try {
        std::unique_ptr<CFFmpegPlayer> player(
            new CFFmpegPlayer(fileName));

        m_ffmpegPlayer = std::move(player);
        SetTextureFormat(GL_BGRA, GL_RGBA);

        // assume framerate is 24 fps
        m_msPerFrame = 1000.f / 24.f;
        return true;
    }
    catch (std::runtime_error &e) {
        std::cout << "The file: '" + fileName + "' seems not exist!" << std::endl
                  << "Internal error message: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown error" << std::endl;
    }
    return false;
}

CFractalTexture::CFractalTexture()
{
}

void CFractalTexture::DoUpdate(CBuffer* buffer)
{
    GenerateFractal(buffer->GetWidth(), buffer->GetHeight(),
                    buffer->GetWorkingBuffer());
}

void CFractalTexture::StopUpdate()
{
    StopGenerate();
}
