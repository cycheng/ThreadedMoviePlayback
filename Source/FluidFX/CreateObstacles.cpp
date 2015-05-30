/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#include "Stdafx.hpp"
#include "Fluid.hpp"
#include <QOpenGLBuffer>
#include <math.h>
#include <iostream>
void CreateObstacles(Surface dest, int width, int height,
                     float xadjuster, float yadjuster,
                     float xpos, float ypos, GLuint program,
                     QOpenGLBuffer* border, QOpenGLBuffer* circle)
{
    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glViewport(0, 0, width, height);
    GL().glClearColor(0, 0, 0, 0);
    GL().glClear(GL_COLOR_BUFFER_BIT);

    GL().glUseProgram(program);

    const int DrawBorder = 1;
    if (DrawBorder && border) {
        #define T 0.9999f
        float positions[] = { -T, -T, T, -T, T,  T, -T,  T, -T, -T };
        #undef T

        border->bind();
        GLsizeiptr size = sizeof(positions);
        border->allocate(positions, size);

        glVertexPointer(2, GL_FLOAT, 0, 0);
        glEnableClientState(GL_VERTEX_ARRAY);
        GLsizeiptr stride = 2 * sizeof(positions[0]);
        GL().glDrawArrays(GL_LINE_STRIP, 0, 5);
    }

    const int DrawCircle = 1;
    if (DrawCircle && circle) {
        const int slices = 64;
        float positions[slices*2*3];
        float twopi = 8*atan(1.0f);
        float theta = 0;
        float dtheta = twopi / (float) (slices - 1);
        float* pPositions = &positions[0];
        float glXpos = 2.f * xpos - 1.f;
        float glYpos = 2.f * ypos - 1.f;

        for (int i = 0; i < slices; i++) {
            *pPositions++ = 0 + glXpos;
            *pPositions++ = 0 + glYpos;

            *pPositions++ = xadjuster * 0.25f * cos(theta) * height / width + glXpos;
            *pPositions++ = yadjuster * 0.25f * sin(theta) + glYpos;
            theta += dtheta;

            *pPositions++ = xadjuster * 0.25f * cos(theta) * height / width + glXpos;
            *pPositions++ = yadjuster * 0.25f * sin(theta) + glYpos;
        }
        GLsizeiptr size = sizeof(positions);
        circle->bind();
        circle->allocate(positions, size);
        glVertexPointer(2, GL_FLOAT, 0, 0);
        glEnableClientState(GL_VERTEX_ARRAY);
        GL().glDrawArrays(GL_TRIANGLES, 0, slices * 3);
    }
}
