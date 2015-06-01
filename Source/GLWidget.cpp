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
                                                               m_threadMode(false), m_bufferMode(BF_SINGLE),
                                                               m_timeStamp(0)
{
}

CGLWidget::~CGLWidget()
{
    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Stop();
            delete t;
        });

    glDeleteTextures(1, &m_lookupTexture);
    DestroyTextureRenderTarget();
}

CGLWidget::PauseWorkers::PauseWorkers(CGLWidget* widget) : m_widget(widget)
{
    if (m_widget->m_threadMode)
    {
        for (auto& worker : m_widget->m_threads)
        {
            worker->Pause();
        }
    }
}

CGLWidget::PauseWorkers::~PauseWorkers()
{
    if (m_widget->m_threadMode)
    {
        for (auto& worker : m_widget->m_threads)
        {
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
            texObj->CopyWorkerDataToMe();
        }
    }
    else
    {
        m_threadMode = true;

        for (auto& texObj : m_threadTextures)
        {
            if (mode == BF_DOUBLE) {
                texObj->GetWorker()->UseDoubleBuffer();
            }
            else {
                texObj->GetWorker()->UseTripleBuffer();
            }
        }

        if (oldmode == BF_SINGLE)
        {
            for (auto& texObj : m_threadTextures)
            {
                texObj->CopyMyDataToWorker();
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

    // initialize texture objects
    CFFmpegPlayer::initFFmpeg();
    m_videoTex.ChangeVideo("../TestVideo/big_buck_bunny_480p_stereo.avi");
    m_fractalTex.SetTextureFormat(GL_RED, GL_R8);
    m_fractalTex.SetAnimated(false);
    m_fractalTex.SetSeedPoint(QPointF(-0.372867, 0.602788));

    // bind texture objects and worker
    m_threads[0]->BindTextureObject(&m_videoTex);
    m_threads[1]->BindTextureObject(&m_fractalTex);
    m_threadTextures.push_back(&m_videoTex);
    m_threadTextures.push_back(&m_fractalTex);

    // initialize effects
    m_effects[FX_BASE] = &m_basefx;
    m_effects[FX_FRACTAL] = &m_fractalfx;
    m_effects[FX_FLUID] = &m_fluidfx;
    m_effects[FX_PAGECURL] = &m_pagecurlfx;

    m_fractalfx.BindTexture(&m_videoTex, &m_fractalTex, m_lookupTexture);
    m_basefx.BindTexture(&m_videoTex);
    m_fluidfx.SetSizeLimit(450, 450);

    for (auto& fx: m_effects)
    {
        fx->InitEffect(this);
        fx->Enable();
    }
    m_basefx.Disable();

    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            t->Pause();
        });

    setMouseTracking(true);
    m_timeStamp = QTime::currentTime().msecsSinceStartOfDay();
}

void CGLWidget::resizeGL(const int width, const int height)
{
    glViewport(0, 0, width, height);

    {
        PauseWorkers pauseWorkers(this);

        std::for_each(m_threadTextures.begin(), m_threadTextures.end(),
            [this, width, height](CTextureObject* texObj) {
                texObj->Resize(width, height);
            });
    }

    DestroyTextureRenderTarget();
    CreateTextureRenderTarget(width, height);

    GLuint rt = m_effects[FX_PAGECURL]->IsEnabled() ? m_fboId : 0;

    for (auto& fx : m_effects)
    {
        fx->WindowResize(width, height);
        fx->SetRenderTarget(rt);
    }

    m_pagecurlfx.SetRenderTarget(0);
    m_pagecurlfx.SetInputTextureId(m_fboTextureId);
}

void CGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);

    int currentMs = QTime::currentTime().msecsSinceStartOfDay();
    int elapsedMs = currentMs - m_timeStamp;

    const CBuffer* updatedBuf = nullptr;
    if (m_threadMode)
    {
        for (auto& texObj: m_threadTextures)
        {
            texObj->UpdateByWorker(elapsedMs);
        }
    }
    else
    {
        for (auto& texObj: m_threadTextures)
        {
            texObj->UpdateByMySelf(elapsedMs);
        }
    }

    m_vertexBuffer->bind();
    for (auto& fx: m_effects)
    {
        fx->Update(elapsedMs);
        fx->Render();
    }

    m_timeStamp = currentMs;
    glDisable(GL_BLEND);
}

void CGLWidget::SetAnimated(int state)
{
    m_fractalTex.SetAnimated(state > 0);
}

void CGLWidget::SetPageCurlAnimated(int state)
{
    m_pagecurlfx.SetAnimated(state > 0);
}

void CGLWidget::ChangeAlphaValue(int alpha)
{
    m_fractalfx.SetAlpha((float)alpha / 100.0f);
}

void CGLWidget::EnableFX(EFFECT id)
{
    m_effects[id]->Enable();

    switch (id)
    {
    case FX_FRACTAL:
        m_basefx.Disable();
        break;
    case FX_PAGECURL:
        for (auto& fx: m_effects)
        {
            fx->SetRenderTarget(m_fboId);
        }
        m_pagecurlfx.SetRenderTarget(0);
        break;
    }
}

void CGLWidget::DisableFX(EFFECT id)
{
    m_effects[id]->Disable();

    switch (id)
    {
    case FX_FRACTAL:
        m_basefx.Enable();
        break;
    case FX_PAGECURL:
        for (auto& fx : m_effects)
        {
            fx->SetRenderTarget(0);
        }
        break;
    }
}

void CGLWidget::NewVideo(const char* filename)
{
    if (m_threadMode)
        m_videoTex.GetWorker()->Pause();

    m_videoTex.ChangeVideo(filename);
    // twice resize because if the width and height equal to original value,
    // we don't resize, so we resize twice to trigger it perform real resize
    m_videoTex.Resize(4, 4);
    m_videoTex.Resize(width(), height());

    if (m_threadMode)
        m_videoTex.GetWorker()->Resume(true);
}

void CGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int xpos = event->pos().rx();
    int ypos = event->pos().ry();

    m_fluidfx.ObstacleCollisionCheck(xpos, ypos);
    if (event->buttons() & Qt::LeftButton) {
        m_fluidfx.SetMousePosition(xpos, ypos);
    }
}

void CGLWidget::CreateTextureRenderTarget(int width, int height)
{
    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (GL_NO_ERROR != glGetError())
    {
        throw std::runtime_error("Unable to create normals texture");
    }

    GLuint colorbuffer;
    glGenRenderbuffers(1, &colorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);

    if (GL_NO_ERROR != glGetError())
    {
        throw std::runtime_error("Unable to create normals texture");
    }

    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
    {
        throw std::runtime_error("Unable to create FBO.");
    }

    m_fboId = fboHandle;
    m_fboTextureId = textureHandle;
    m_renderBufferId = colorbuffer;

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CGLWidget::DestroyTextureRenderTarget()
{
    if (! m_fboId)
        return;

    glDeleteFramebuffers(1, &m_fboId);
    glDeleteTextures(1, &m_fboTextureId);
    glDeleteRenderbuffers(1, &m_renderBufferId);
}

QOpenGLFunctions& GL()
{
    return *CGLWidget::m_glProvider;
}
