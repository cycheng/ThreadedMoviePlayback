#include "Stdafx.hpp"
#include "Effect.hpp"
#include "GLWidget.hpp"

CEffect::CEffect(): m_parent(nullptr), m_enabled(true),
                    m_width(0), m_height(0)
{
}

CEffect::~CEffect()
{
}

void CEffect::InitEffect(QObject* parent)
{
    m_parent = parent;
}

bool CEffect::WindowResize(int width, int height)
{
    if (m_width == width && m_height == height)
        return false;

    m_width = width;
    m_height = height;
    return true;
}

void CEffect::Enable()
{
    m_enabled = true;
}

void CEffect::Disable()
{
    m_enabled = false;
}

void CEffect::Update()
{
    if (! m_enabled)
        return;

    DoUpdate();
}

void CEffect::Render()
{
    if (m_enabled)
        DoRender();
}

// ----------------------------------------------------------------------------
// Fractal Effect
// ----------------------------------------------------------------------------
#include "Fractal.hpp"
#include "FFmpegPlayer.hpp"

CFractalFX::CFractalFX(): m_fractalLoc(-1), m_ffmpegLoc(-1), m_alphaLoc(-1),
                          m_alpha(0.f),
                          m_videoTex(nullptr), m_fractalTex(nullptr)
{
}

CFractalFX::~CFractalFX()
{
}

void CFractalFX::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
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

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
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

    m_program.setParent(parent);
    m_program.addShader(vshader);
    m_program.addShader(fshader);
    m_program.bindAttributeLocation("vertex", 0);
    m_program.link();

    m_fractalLoc = m_program.uniformLocation("fractalTex");
    m_ffmpegLoc = m_program.uniformLocation("videoTex");
    m_alphaLoc = m_program.uniformLocation("alpha");
}

void CFractalFX::BindTexture(CVideoTexture* video, CFractalTexture* fractal)
{
    m_videoTex = video;
    m_fractalTex = fractal;
}

void CFractalFX::SetAlpha(float alpha)
{
    m_alpha = alpha;
}

void CFractalFX::DoUpdate()
{
    /* Video and fractal textures are updated by CGLWidget::paintGL */
}

void CFractalFX::DoRender()
{
    glViewport(0, 0, m_width, m_height);
    m_program.bind();

    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, m_fractalTex->GetTextureID());
    m_program.setUniformValue(m_fractalLoc, 0);

    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, m_videoTex->GetTextureID());
    m_program.setUniformValue(m_ffmpegLoc, 1);

    m_program.setUniformValue(m_alphaLoc, m_alpha);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, 0);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, 0);
}

// ----------------------------------------------------------------------------
// FluidFX Effect
// ----------------------------------------------------------------------------
#include "FluidFX/Fluid.hpp"

/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

void CFluidFX::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);
    FluidInit(parent);
}

bool CFluidFX::WindowResize(int width, int height)
{
    if (! CEffect::WindowResize(width, height))
        return false;

    if (width > 500) width = 500;
    if (height > 500) height = 500;

    m_width = width;
    m_height = height;
    FluidResize(width, height);

    return true;
}

void CFluidFX::DoUpdate()
{
    FluidUpdate(0);
}

void CFluidFX::DoRender()
{
    FluidRender(0, m_width, m_height);
}
