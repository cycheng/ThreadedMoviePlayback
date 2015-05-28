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
    m_effects.push_back(&m_fractalfx);
    m_effects.push_back(&m_fluidfx);

    for (auto& fx: m_effects)
    {
        fx->InitEffect(this);
    }

    m_fractalfx.BindTexture(&m_videoTex, &m_fractalTex);
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
            });
    }

    for (auto& fx : m_effects)
        fx->WindowResize(width, height);
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
            texObj->UpdateByWorker();
        }
    }
    else
    {
        for (auto& texObj : m_threadTextures)
        {
            texObj->UpdateByMySelf();
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
        m_fractalTex.SetAnimated(true);
    else
        m_fractalTex.SetAnimated(false);
}

void CGLWidget::ChangeAlphaValue(int alpha)
{
    m_fractalfx.SetAlpha((float)alpha / 100.0f);
}

QOpenGLFunctions& GL()
{
    return *CGLWidget::m_glProvider;
}
