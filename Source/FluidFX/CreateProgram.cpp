#include "Stdafx.hpp"
#include "Fluid.hpp"
#include "../GLWidget.hpp"

#include <QFile>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

GLuint CreateProgram(QObject* parent, const char* fsKey)
{
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
    QFile vsrc(":/CMainWindow/Fluid.vert");
    vsrc.open(QIODevice::ReadOnly | QIODevice::Text);
    vshader->compileSourceCode(vsrc.readAll());

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
    QFile fsrc(QString(":/CMainWindow/") + fsKey);
    fsrc.open(QIODevice::ReadOnly | QIODevice::Text);
    fshader->compileSourceCode(fsrc.readAll());

    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(parent);
    program->addShader(vshader);
    program->addShader(fshader);
    program->link();

    return program->programId();
}
