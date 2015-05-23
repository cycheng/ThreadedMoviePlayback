#include "Stdafx.hpp"
#include "TextureObject.hpp"
#include "FFmpegPlayer.hpp"
#include "Fractal.hpp"
#include "GLWidget.hpp"
#include "FluidFX.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

QOpenGLFunctions* CGLWidget::m_glProvider = nullptr;

CGLWidget::CGLWidget(QWidget* parent, QGLWidget* shareWidget): QGLWidget(parent, shareWidget),
                                                               m_lookupTexture(0),
                                                               m_alpha(0.f),
                                                               m_program(nullptr), m_vertexBuffer(nullptr),
                                                               m_threadMode(false), m_bufferMode(BF_SINGLE)
{
}

CGLWidget::~CGLWidget()
{
    std::for_each(m_threads.begin(), m_threads.end(),
        [](CWorker* t) {
            //t->Resume(false);
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

        for (auto& texObj : m_textures)
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
            for (auto& texObj : m_textures)
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
        "uniform float alpha;\n"
        "void main(void)\n"
        "{\n"
        "    vec4 fractColour = texture2D(fractalTex, texc);\n"
        "    float fractAlpha = fractColour.r * (1.0 - alpha);"
        "\n"
        "    vec4 videoColour = texture2D(videoTex, texc);\n"
        "\n"
        "    gl_FragColor = fractColour * fractAlpha +\n"
        "                   videoColour * (1.0 - fractAlpha);\n"
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
    m_alphaLoc = m_program->uniformLocation("alpha");

    m_threads.push_back(new CWorker);
    m_threads.push_back(new CWorker);
    //m_threads.push_back(new CWorker);

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

    m_fractalTex->GetFractal()->SetAnimated(false);
    m_fractalTex->GetFractal()->SetSeedPoint(QPointF(-0.372867, 0.602788));

    m_fluidTex.reset(new CFluidFXTexture(this));
    m_fluidTex->SetTextureFormat(GL_RED, GL_R8);
    //m_fluidTex->BindWorker(m_threads[2]);

    m_textures.push_back(m_videoTex.get());
    m_textures.push_back(m_fractalTex.get());
    //m_textures.push_back(m_fluidTex.get());

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
    m_fluidTex->Resize(width, height);
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
    m_fluidTex->Update();

    glViewport(0, 0, width(), height());
    m_program->bind();
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

    m_program->setUniformValue(m_alphaLoc, m_alpha);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_vertexBuffer->bind();

    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_fluidTex->Render();
}

void CGLWidget::SetAnimated(int state)
{
    if (state > 0)
        m_fractalTex->GetFractal()->SetAnimated(true);
    else
        m_fractalTex->GetFractal()->SetAnimated(false);
}

void CGLWidget::ChangeAlphaValue(int alpha)
{
    m_alpha = (float)alpha / 100.0f;
}
