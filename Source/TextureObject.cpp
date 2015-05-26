#include "Stdafx.hpp"
#include "TextureObject.hpp"
#include "FFmpegPlayer.hpp"
#include "Fractal.hpp"

#include <cassert>
#include <iostream>

CWorker::CWorker() : m_pause(true), m_stop(false), m_restart(false),
m_inPauseState(false), m_inSwapWaitState(false),
m_buffer(nullptr), m_texObj(nullptr)
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
        m_texObj->DoUpdate(m_buffer.get());

        {
            QMutexLocker locker(&m_mutex);

            if (m_pause) {
                m_pauseSignal.wakeOne();
                m_inPauseState = true;
                m_runSignal.wait(&m_mutex);
                m_inPauseState = false;
            }

            if (m_stop) {
                m_pauseSignal.wakeOne();
                break;
            }

            if (!m_buffer->CanWeSwapWorkingBuffer()) {
                m_inSwapWaitState = true;
                m_swapBufferSignal.wait(&m_mutex);
                m_inSwapWaitState = false;
            }

            // Check if we need to drop this frame **BEFORE** we swap buffer.
            if (m_restart) {
                m_restart = false;
                continue;
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

    if (m_inPauseState || m_inSwapWaitState) {
        return;
    }

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

    if (m_inSwapWaitState) {
        m_swapBufferSignal.wakeOne();
    }

    if (m_inPauseState) {
        m_pause = false;
        m_runSignal.wakeOne();
    }

    m_pauseSignal.wait(&m_mutex);
}

void CWorker::UseTripleBuffer()
{
    // todo !!
    assert(m_buffer == false);

    m_buffer.reset(new CTripleBuffer);
}

void CWorker::BindTextureObject(CTextureObject* texObj)
{
    m_texObj = texObj;
    m_buffer->SetPixelSize(texObj->GetBuffer()->GetPixelSize());
}

CBuffer* CWorker::GetInternalBuffer()
{
    return m_buffer.get();
}

CTextureObject::CTextureObject() : m_worker(nullptr), m_textureId(0),
m_bufferFmt(0), m_internalFmt(0)
{
}

CTextureObject::~CTextureObject()
{
    glDeleteTextures(1, &m_textureId);
}

void CTextureObject::Resize(int width, int height)
{
    if (m_buffer.GetWidth() == width && m_buffer.GetHeight() == height) {
        return;
    }

    if (m_worker) {
        m_worker->GetInternalBuffer()->SetTextureSize(width, height);
        m_worker->GetInternalBuffer()->InitIntermediateBufferWithZero();
    }
    m_buffer.SetTextureSize(width, height);
    m_buffer.InitIntermediateBufferWithZero();
    // CreateTexture is called by external
}

void CTextureObject::StopUpdate()
{
}

void CTextureObject::Update()
{
    DoUpdate(&m_buffer);
}

void CTextureObject::BindWorker(CWorker* worker)
{
    worker->BindTextureObject(this);
    m_worker = worker;
}

void CTextureObject::SetTextureFormat(GLenum bufferFmt, GLint internalFmt)
{
    m_bufferFmt = bufferFmt;
    m_internalFmt = internalFmt;
    m_buffer.SetPixelSize(GetGLPixelSize(bufferFmt));
}

void CTextureObject::SetNewTextureId(GLuint newGLId)
{
    m_textureId = newGLId;
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

GLenum CTextureObject::GetBufferFormat() const
{
    return m_bufferFmt;
}

GLint CTextureObject::GetInternalFormat() const
{
    return m_internalFmt;
}

void CVideoTexture::DoUpdate(CBuffer* buffer)
{
    unsigned int pts;
    bool newframe = false;

    while (!newframe) {
        newframe = m_ffmpegPlayer->decodeFrame(
            pts,
            buffer->GetWorkingBuffer(), buffer->GetRowSize());
    }
}

void CVideoTexture::Resize(int width, int height)
{
    if (m_buffer.GetWidth() == width && m_buffer.GetHeight() == height) {
        return;
    }

    CTextureObject::Resize(width, height);
    m_ffmpegPlayer->setOutputSize(width, height);

    // decode one frame to initialize result buffer
    Update();
    if (m_worker) {
        m_worker->GetInternalBuffer()->InitIntermediateBuffer(
            m_buffer.GetWorkingBuffer(), m_buffer.GetSize());
    }
}

bool CVideoTexture::ChangeVideo(const std::string& fileName)
{
    try {
        std::unique_ptr<CFFmpegPlayer> player(
            new CFFmpegPlayer(fileName));

        m_ffmpegPlayer = std::move(player);
        SetTextureFormat(GL_BGRA, GL_RGBA);

        return true;
    }
    catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown error" << std::endl;
    }
    return false;
}

CFractalTexture::CFractalTexture()
{
    //m_fractal.reset(new CFractal);
}

void CFractalTexture::DoUpdate(CBuffer* buffer)
{
    //m_fractal->
    GenerateFractal(buffer->GetWidth(), buffer->GetHeight(),
                    buffer->GetWorkingBuffer());
}

void CFractalTexture::StopUpdate()
{
    StopGenerate();
}

/*CFractal* CFractalTexture::GetFractal()
{
    return m_fractal.get();
}*/
