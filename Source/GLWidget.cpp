#include "Stdafx.hpp"
#include "GLWidget.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <iostream>

#include "FFmpegPlayer.hpp"
#include "Buffer.hpp"

CGLWidget::CGLWidget(QWidget* parent, QGLWidget* shareWidget): QGLWidget(parent, shareWidget), m_fractalTexture(0),
                                                               m_ffmpegPlayerTexture(0), m_lookupTexture(0),
                                                               m_program(nullptr), m_vertexBuffer(nullptr)
{
}

CGLWidget::~CGLWidget()
{
    glDeleteTextures(1, &m_fractalTexture);
    glDeleteTextures(1, &m_ffmpegPlayerTexture);
    glDeleteTextures(1, &m_lookupTexture);
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

    CFFmpegPlayer::initFFmpeg();
    try {
        std::unique_ptr<CFFmpegPlayer> player(
            new CFFmpegPlayer("../TestVideo/big_buck_bunny_480p_stereo.avi"));

        m_ffmpegPlayer = std::move(player);
    }
    catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown error" << std::endl;
    }
}

void upateVideo(CFFmpegPlayer* video, CBuffer& buf) {
    unsigned int pts;
    bool newframe = false;

    while (! newframe) {
        newframe = video->decodeFrame(pts, buf.GetPtr(), buf.GetRowSize());
    }
}

void CGLWidget::resizeGL(const int width, const int height)
{
    glViewport(0, 0, width, height);
    m_ffmpegPlayerBuf.SetTextureSize(width, height);
    m_ffmpegPlayer->setOutputSize(width, height);

    m_fractalBuf.SetTextureSize(width, height, FRACTAL_ELEM_1_BYTE);
}

bool CGLWidget::UpdateTexture(const CBuffer& buffer, GLuint& texture,
    GLenum bufferFormat, GLint internalFormat/*, int& textureWidth, int& textureHeight*/)
{
    // TODO: Find a way to bind ptr to the image buffer produced
    void* ptr = buffer.GetPtr();
    const int textureWidth = buffer.GetWidth();
    const int textureHeight = buffer.GetHeight();

    //glActiveTexture(GL_TEXTURE0);

    if (texture != 0)
    {
        glDeleteTextures(1, &texture);
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, textureWidth, textureHeight, 0, bufferFormat, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, bufferFormat, GL_UNSIGNED_BYTE, ptr);

	return true;
}

void CGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    upateVideo(m_ffmpegPlayer.get(), m_ffmpegPlayerBuf);
    // TODO: Call UpdateTexture() here to produce fractal and ffmpegPlayer textures
    UpdateTexture(m_ffmpegPlayerBuf, m_ffmpegPlayerTexture, GL_BGRA, GL_RGBA);

    m_fractal.GenerateFractal(m_fractalBuf);
    UpdateTexture(m_fractalBuf, m_fractalTexture, GL_RED, GL_R8);
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
    glBindTexture(GL_TEXTURE_2D, m_fractalTexture);
    m_program->setUniformValue(m_fractalLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ffmpegPlayerTexture);
    m_program->setUniformValue(m_ffmpegLoc, 1);
#endif

    m_vertexBuffer->bind();
    m_program->bind();
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}