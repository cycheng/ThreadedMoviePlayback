#include "Stdafx.hpp"
#include "GLWidget.hpp"
#include "FFmpegPlayer.hpp"     // CFFmpeg::initFFmpeg
#include "Fractal.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

QOpenGLFunctions* CGLWidget::m_glProvider = nullptr;

CGLWidget::CGLWidget(QWidget* parent, QGLWidget* shareWidget): QGLWidget(parent, shareWidget),
                                                               m_lookupTexture(0),
                                                               m_vertexBuffer(nullptr),
                                                               m_threadMode(false), m_bufferMode(BF_SINGLE)
{
}

CGLWidget::~CGLWidget()
{
    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Stop();
        });

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

    PauseWorkers pauseWorkers(this);

    BUFFER_MODE oldmode = m_bufferMode;
    m_bufferMode = mode;

    if (mode == BF_SINGLE)
    {
        m_threadMode = false;

        for (auto& texObj : m_threadTextures)
        {
            const CBuffer* src = texObj->GetWorker()->GetInternalBuffer();
            CBuffer* dest = texObj->GetBuffer();
            dest->InitIntermediateBuffer(src->GetIntermediateBuffer(),
                                         src->GetSize());
        }
    }
    else {
        m_threadMode = true;

        if (oldmode == BF_SINGLE)
        {
            for (auto& texObj : m_threadTextures)
            {
                const CBuffer* src = texObj->GetBuffer();
                CBuffer* dest = texObj->GetWorker()->GetInternalBuffer();
                dest->InitIntermediateBuffer(src->GetIntermediateBuffer(),
                                             src->GetSize());
            }
        }
    }
}

void CGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0, 1.0, 0.0, 0.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    makeCurrent();

    CGLWidget::m_glProvider = this;

    QImage image(":/CMainWindow/Resources/lookup.png");
    m_lookupTexture = bindTexture(image);

    m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vertexBuffer->create();
    m_vertexBuffer->bind();
    float data[8] = {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f};
    m_vertexBuffer->allocate(data, sizeof(data));

    m_threads.push_back(new CWorker);
    m_threads.push_back(new CWorker);

    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* w) {
            w->UseTripleBuffer();
            w->start();
        });

    m_effects.push_back(&m_fractalfx);
    m_effects.push_back(&m_fluidfx);

    for (auto& fx: m_effects)
        fx->InitEffect(this);

    CFFmpegPlayer::initFFmpeg();
    m_fractalfx.GetVideoTexture()->ChangeVideo("../TestVideo/big_buck_bunny_480p_stereo.avi");
    m_fractalfx.GetVideoTexture()->BindWorker(m_threads[0]);

    m_fractalfx.GetFractalTexture()->BindWorker(m_threads[1]);

    m_threadTextures.push_back(m_fractalfx.GetVideoTexture());
    m_threadTextures.push_back(m_fractalfx.GetFractalTexture());

    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Pause();
        });
}

void CGLWidget::resizeGL(const int width, const int height)
{
    glViewport(0, 0, width, height);
    {
        PauseWorkers pauseWorkers(this);

        std::for_each(m_threadTextures.begin(), m_threadTextures.end(),
            [this, width, height](CTextureObject* texObj) {
            texObj->Resize(width, height);
            CreateTexture(texObj);
        });

    }

    for (auto& fx : m_effects)
        fx->WindowResize(width, height);
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
    glEnable(GL_BLEND);

    const CBuffer* updatedBuf = nullptr;
    if (m_threadMode)
    {
        for (auto& texObj : m_threadTextures)
        {
            const CBuffer* updatedBuf =
                texObj->GetWorker()->GetUpdatedBufferAndSignalWorker();
            UpdateTexture(texObj, updatedBuf);
        }
    }
    else {
        for (auto& texObj : m_threadTextures)
        {
            texObj->Update();
            UpdateTexture(texObj, texObj->GetBuffer());
        }
    }

    m_vertexBuffer->bind();
    for (auto& fx : m_effects)
    {
        fx->Update();
        fx->Render();
    }
}

void CGLWidget::SetAnimated(int state)
{
    if (state > 0)
        m_fractalfx.GetFractalTexture()->SetAnimated(true);
    else
        m_fractalfx.GetFractalTexture()->SetAnimated(false);
}

void CGLWidget::ChangeAlphaValue(int alpha)
{
    m_fractalfx.SetAlpha((float)alpha / 100.0f);
}

QOpenGLFunctions& GL()
{
    return *CGLWidget::m_glProvider;
}
