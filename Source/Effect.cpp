#include "Stdafx.hpp"
#include "Effect.hpp"
#include "GLWidget.hpp"

CEffect::CEffect(): m_parent(nullptr), m_enabled(true),
                    m_width(0), m_height(0), m_renderTarget(0)
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
    {
        return false;
    }
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

void CEffect::Update(int elapsedMs)
{
    if (m_enabled)
        DoUpdate(elapsedMs);
}

void CEffect::Render()
{
    if (m_enabled)
        DoRender();
}

void CEffect::SetRenderTarget(GLuint rt)
{
    m_renderTarget = rt;
}

bool CEffect::IsEnabled() const
{
    return m_enabled;
}

static const char *BASIC_VERTEX_SHADER =
    "attribute highp vec4 vertex;\n"
    "varying mediump vec2 texc;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = vec4(vertex.xy, 0.0, 1.0);\n"
    "    texc.x = 0.5 * (1.0 + vertex.x);\n"
    "    texc.y = 0.5 * (1.0 - vertex.y);\n"
    "}\n";

// ----------------------------------------------------------------------------
// Base Effect: movie playback
// ----------------------------------------------------------------------------
CMoviePlayback::CMoviePlayback(): m_videoTex(nullptr), m_ffmpegLoc(-1)
{
}

void CMoviePlayback::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
    vshader->compileSourceCode(BASIC_VERTEX_SHADER);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
    const char *fsrc =
        "varying mediump vec2 texc;\n"
        "uniform sampler2D videoTex;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(videoTex, texc);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    m_program.setParent(parent);
    m_program.addShader(vshader);
    m_program.addShader(fshader);
    m_program.bindAttributeLocation("vertex", 0);
    m_program.link();

    m_ffmpegLoc = m_program.uniformLocation("videoTex");
}

void CMoviePlayback::Enable()
{
    CEffect::Enable();
    m_videoTex->Enable();
}

void CMoviePlayback::Disable()
{
    CEffect::Disable();
    m_videoTex->Disable();
}

void CMoviePlayback::BindTexture(CVideoTexture* video)
{
    m_videoTex = video;
}

void CMoviePlayback::DoUpdate(int elapsedMs)
{
    /* Video textures is updated by CGLWidget::paintGL */
}

void CMoviePlayback::DoRender()
{
    glViewport(0, 0, m_width, m_height);
    m_program.bind();

    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, m_videoTex->GetTextureID());
    m_program.setUniformValue(m_ffmpegLoc, 0);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, m_renderTarget);

    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, 0);
}

// ----------------------------------------------------------------------------
// Fractal Effect
// ----------------------------------------------------------------------------
#include "Fractal.hpp"
#include "FFmpegPlayer.hpp"

CFractalFX::CFractalFX(): m_fractalLoc(-1), m_ffmpegLoc(-1), m_alphaLoc(-1),
                          m_lookupLoc(-1), m_alpha(0.f),
                          m_videoTex(nullptr), m_fractalTex(nullptr), m_lookupTexId(0)
{
}

CFractalFX::~CFractalFX()
{
}

void CFractalFX::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
    vshader->compileSourceCode(BASIC_VERTEX_SHADER);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
    const char *fsrc =
        "varying mediump vec2 texc;\n"
        "uniform sampler2D fractalTex;\n"
        "uniform sampler2D videoTex;\n"
        "uniform sampler2D lookupTex;\n"
        "uniform float alpha;\n"
        "void main(void)\n"
        "{\n"
        "    vec4 fractColour = texture2D(fractalTex, texc);\n"
        "    float fractAlpha = fractColour.r * (1.0 - alpha);\n"
        "\n"
        "    vec4 videoColour = texture2D(videoTex, texc);\n"
        "    vec4 lookupColour = texture2D(lookupTex, vec2(fractColour.x, 0.0));\n"
        "\n"
        "    gl_FragColor = lookupColour * fractAlpha +\n"
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
    m_lookupLoc = m_program.uniformLocation("lookupTex");
    m_alphaLoc = m_program.uniformLocation("alpha");
}

void CFractalFX::Enable()
{
    CEffect::Enable();
    m_videoTex->Enable();
    m_fractalTex->Enable();
}

void CFractalFX::Disable()
{
    CEffect::Disable();
    m_videoTex->Disable();
    m_fractalTex->Disable();
}

void CFractalFX::BindTexture(CVideoTexture* video, CFractalTexture* fractal, GLuint lookup)
{
    m_videoTex = video;
    m_fractalTex = fractal;
    m_lookupTexId = lookup;
}

void CFractalFX::SetAlpha(float alpha)
{
    m_alpha = alpha;
}

void CFractalFX::DoUpdate(int elapsedMs)
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

    GL().glActiveTexture(GL_TEXTURE2);
    GL().glBindTexture(GL_TEXTURE_2D, m_lookupTexId);
    m_program.setUniformValue(m_lookupLoc, 2);

    m_program.setUniformValue(m_alphaLoc, m_alpha);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, m_renderTarget);

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

CFluidFX::CFluidFX(): m_widthLimit(0), m_heightLimit(0),
                      m_mouseX(0.5f), m_mouseY(0.5f), m_mouseOnObstacle(false)
{
}

CFluidFX::~CFluidFX()
{
    FluidUninit();
}

void CFluidFX::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);
    FluidInit(parent);
}

bool CFluidFX::WindowResize(int width, int height)
{
    if (! CEffect::WindowResize(width, height))
        return false;

    if (width > m_widthLimit && height > m_heightLimit)
    {
        // re-create obstacle to maintain correct visual ratio, so
        // the circle won't looks like ellipsoid, and put obstacle in
        // correct position.
        //
        // So, don't return here, even though our real textures size
        // are unchanged. Re-create all to avoid strange result.
    }

    FluidResize(RealWidth(), RealHeight());
    FluidObstacleResize(m_mouseX, m_mouseY, RealWidth(), RealHeight(), AdjustX(), AdjustY());

    return true;
}

void CFluidFX::SetMousePosition(int xpos, int ypos)
{
    m_mouseX = (float)xpos / m_width;
    m_mouseY = (float)ypos / m_height;

    // Only need to update circle position
    FluidSetCirclePosition(m_mouseX, m_mouseY, RealWidth(), RealHeight(), AdjustX(), AdjustY());
}

void CFluidFX::SetSizeLimit(int maxwidth, int maxheight)
{
    if (maxwidth > 0) {
        m_widthLimit = maxwidth;
    }

    if (maxheight > 0) {
        m_heightLimit = maxheight;
    }
}

void CFluidFX::ObstacleCollisionCheck(int xpos, int ypos)
{
    float relx = (float)xpos / m_width;
    float rely = (float)ypos / m_height;

    float dx = m_mouseX - relx;
    float dy = m_mouseY - rely;
    // roughly distance check !
    float collideDist = 0.01f * (AdjustX() * AdjustX() + AdjustY() * AdjustY());

    m_mouseOnObstacle = false;
    if (dx*dx + dy*dy <= collideDist)
    {
        m_mouseOnObstacle = true;
    }
}

float CFluidFX::AdjustX() const
{
    if (m_widthLimit < m_width)
    {
        return (float)m_widthLimit / m_width;
    }
    return 1.0f;
}

float CFluidFX::AdjustY() const
{
    if (m_heightLimit < m_height)
    {
        return (float)m_heightLimit / m_height;
    }
    return 1.0f;
}

int CFluidFX::RealWidth() const
{
    return std::min(m_width, m_widthLimit);
}

int CFluidFX::RealHeight() const
{
    return std::min(m_height, m_heightLimit);
}

void CFluidFX::DoUpdate(int elapsedMs)
{
    FluidUpdate(0);
}

void CFluidFX::DoRender()
{
    FluidRender(m_renderTarget, m_width, m_height, !m_mouseOnObstacle);
}

// ----------------------------------------------------------------------------
// Page-Curl Effect
// ----------------------------------------------------------------------------
CPageCurlFX::CPageCurlFX(): m_time(0.f), m_animated(true),
                            m_vertexLoc(-1), m_sourceTexLoc(-1),
                            m_targetTexLoc(-1), m_timeLoc(-1)
{
}

void CPageCurlFX::InitEffect(QObject* parent)
{
    CEffect::InitEffect(parent);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
    // Flip y coordinate for input texture (render to texture with framebuffer)
    const char* vsrc =
        "attribute highp vec4 vertex;\n"
        "varying mediump vec2 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vec4(vertex.xy, 0.0, 1.0);\n"
        "    texc.x = 0.5 * (1.0 + vertex.x);\n"
        "    texc.y = 0.5 * (1.0 + vertex.y);\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
    QFile frag(":/CMainWindow/page-curl.frag");
    frag.open(QIODevice::ReadOnly | QIODevice::Text);
    fshader->compileSourceCode(frag.readAll());

    m_program.setParent(parent);
    m_program.addShader(vshader);
    m_program.addShader(fshader);
    m_program.bindAttributeLocation("vertex", 0);
    m_program.link();

    m_vertexLoc = m_program.attributeLocation("vertex");
    m_sourceTexLoc = m_program.uniformLocation("sourceTex");
    m_targetTexLoc = m_program.uniformLocation("targetTex");
    m_timeLoc = m_program.uniformLocation("time");
}

void CPageCurlFX::SetInputTextureId(GLuint texid)
{
    m_textureId = texid;
}

void CPageCurlFX::DoUpdate(int elapsedMs)
{
    if (! m_animated)
        return;

    // scroll a page every 2 second.
    const int msPerFrame = 2000;
    if (elapsedMs > msPerFrame)
        elapsedMs = msPerFrame;

    m_time += (float)elapsedMs / (float)msPerFrame;
    if (m_time >= 1.0f)
        m_time -= 1.0f;
}

void CPageCurlFX::DoRender()
{
    glViewport(0, 0, m_width, m_height);
    m_program.bind();

    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, m_textureId);
    m_program.setUniformValue(m_sourceTexLoc, 0);

    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, m_textureId);
    m_program.setUniformValue(m_targetTexLoc, 1);

    m_program.setUniformValue(m_timeLoc, m_time);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, m_renderTarget);

    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, 0);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, 0);
}

void CPageCurlFX::SetAnimated(bool animated)
{
    m_animated = animated;
}

