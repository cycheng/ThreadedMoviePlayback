#include "Stdafx.hpp"
#include "GLWidget.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <iostream>

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
        "    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "    texc = vec2(0.0, 0.0);\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "varying mediump vec2 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShader(vshader);
    m_program->addShader(fshader);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->link();

    m_program->bind();
}

void CGLWidget::resizeGL(const int width, const int height)
{
    glViewport(0, 0, width, height);
}

bool CGLWidget::UpdateTexture(/* buffer, */ GLuint& texture, int& textureWidth, int& textureHeight)
{
    // TODO: Find a way to bind ptr to the image buffer produced
    void* ptr = nullptr;

    glActiveTexture(GL_TEXTURE0);

    if (texture != 0)
    {
        glDeleteTextures(1, &texture);
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, textureWidth, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RED, GL_UNSIGNED_BYTE, ptr);

	return true;
}

void CGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Call UpdateTexture() here to produce fractal and ffmpegPlayer textures

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
    m_vertexBuffer->bind();
    m_program->bind();
    glVertexPointer(2, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}