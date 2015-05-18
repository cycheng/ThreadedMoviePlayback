#include "Stdafx.hpp"
#include "GLWidget.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <iostream>
#include <assert.h>

#include "FFmpegPlayer.hpp"
#include "Buffer.hpp"

CGLWidget::CGLWidget(QWidget* parent, QGLWidget* shareWidget): QGLWidget(parent, shareWidget), m_fractalTexture(0),
                                                               m_ffmpegPlayerTexture(0), m_lookupTexture(0),
                                                               m_program(nullptr), m_vertexBuffer(nullptr),
                                                               m_threadMode(false), m_bufferMode(BF_SINGLE)
{
}

CGLWidget::~CGLWidget()
{
    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Resume(false);
            t->Stop();
        });

    glDeleteTextures(1, &m_fractalTexture);
    glDeleteTextures(1, &m_ffmpegPlayerTexture);
    glDeleteTextures(1, &m_lookupTexture);
}

CGLWidget::PauseWorkers::PauseWorkers(CGLWidget* widget) : m_widget(widget)
{
    if (m_widget->m_threadMode) {
        for (auto& worker : m_widget->m_threads) {
            worker->Pause();
        }
    }
}

CGLWidget::PauseWorkers::~PauseWorkers()
{
    if (m_widget->m_threadMode) {
        for (auto& worker : m_widget->m_threads) {
            worker->Resume(true);
        }
    }
}

void CGLWidget::ChangeBufferMode(BUFFER_MODE mode)
{
    if (m_bufferMode == mode)
        return;

    m_bufferMode = mode;
    PauseWorkers pauseWorkers(this);

    m_threadMode = true;
    if (m_bufferMode == BF_SINGLE) {
        m_threadMode = false;
    }
}

void CGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 1.0, 0.0, 0.0);
    makeCurrent();
    QImage image(":/CMainWindow/Resources/lookup.png");
    m_lookupTexture = bindTexture(image);

    m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vertexBuffer->create();
    m_vertexBuffer->bind();
    float data[8] = {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f};
    m_vertexBuffer->allocate(data, sizeof(data));

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "varying mediump vec2 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vec4(vertex.xy, 0.0, 1.0);\n"
        "    texc.x = 0.5 * (1.0 + vertex.x);\n"
        "    texc.y = 0.5 * (1.0 - vertex.y);\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "varying mediump vec2 texc;\n"
        "uniform sampler2D fractalTex;\n"
        "uniform sampler2D videoTex;\n"
        "void main(void)\n"
        "{\n"
        "    vec4 fractColour = texture2D(fractalTex, texc);\n"
        "    vec4 videoColour = texture2D(videoTex, texc);\n"
        "    gl_FragColor = fractColour.rrrr + videoColour;\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->link();

    m_program->bind();

    m_fractalLoc = m_program->uniformLocation("fractalTex");
    m_ffmpegLoc = m_program->uniformLocation("videoTex");

    m_threads.push_back(new CWorker);
    m_threads.push_back(new CWorker);

    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* w) {
        w->UseTripleBuffer();
        w->start();
    });

    CFFmpegPlayer::initFFmpeg();
    m_videoTex.reset(new CVideoTexture);
    m_videoTex->ChangeVideo("../TestVideo/big_buck_bunny_480p_stereo.avi");
    m_videoTex->BindWorker(m_threads[0]);

    m_fractalTex.reset(new CFractalTexture);
    m_fractalTex->SetTextureFormat(GL_RED, GL_R8);
    m_fractalTex->BindWorker(m_threads[1]);

    m_textures.push_back(m_videoTex.get());
    m_textures.push_back(m_fractalTex.get());

    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Pause();
        });
}

void CGLWidget::resizeGL(const int width, const int height)
{
    glViewport(0, 0, width, height);

    PauseWorkers pauseWorkers(this);

    std::for_each(m_textures.begin(), m_textures.end(),
        [this, width, height](CTextureObject* texObj) {
            texObj->Resize(width, height);
            CreateTexture(texObj);
        });
}

void CGLWidget::CreateTexture(CTextureObject* texObj)
{
    CBuffer* buf = texObj->GetBuffer();
    GLuint textureId = texObj->GetTextureID();

    if (textureId != 0)
    {
        glDeleteTextures(1, &textureId);
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, texObj->GetInternalFormat(),
                 buf->GetWidth(), buf->GetHeight(),
                 0, texObj->GetBufferFormat(), GL_UNSIGNED_BYTE, nullptr);

    texObj->SetNewTextureId(textureId);
}

void CGLWidget::UpdateTexture(const CTextureObject* texObj, const CBuffer* buf)
{
    void* data = buf->GetStableBuffer();

    glBindTexture(GL_TEXTURE_2D, texObj->GetTextureID());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf->GetWidth(), buf->GetHeight(),
                    texObj->GetBufferFormat(), GL_UNSIGNED_BYTE, data);
}

void CGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const CBuffer* updatedBuf = nullptr;
    if (m_threadMode)
    {
        for (auto& texObj : m_textures)
        {
            const CBuffer* updatedBuf =
                texObj->GetWorker()->GetUpdatedBufferAndSignalWorker();
            UpdateTexture(texObj, updatedBuf);
        }
    }
    else {
        for (auto& texObj : m_textures)
        {
            texObj->Update();
            UpdateTexture(texObj, texObj->GetBuffer());
        }
    }

#if 0
    if ((m_fractalTexture == 0) || (m_ffmpegPlayerTexture == 0) || (m_lookupTexture == 0))
    {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fractalTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_lookupTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_ffmpegPlayerTexture);
#else
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fractalTex->GetTextureID());
    m_program->setUniformValue(m_fractalLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_videoTex->GetTextureID());
    m_program->setUniformValue(m_ffmpegLoc, 1);
#endif

    m_vertexBuffer->bind();
    m_program->bind();
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

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

            if (! m_buffer->CanWeSwapWorkingBuffer()) {
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

    while (! newframe) {
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
    m_fractal.reset(new CFractal);
}

void CFractalTexture::DoUpdate(CBuffer* buffer)
{
    m_fractal->GenerateFractal(buffer->GetWidth(), buffer->GetHeight(),
                               buffer->GetWorkingBuffer());
}
