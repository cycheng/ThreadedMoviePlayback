#include "Stdafx.hpp"
#include "Fluid.hpp"

GLuint CreateQuad()
{
#if 0
    short positions[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };

    // Create the VAO:
    GLuint vao;
    GL().glGenVertexArrays(1, &vao);
    GL().glBindVertexArray(vao);

    // Create the VBO:
    GLuint vbo;
    GLsizeiptr size = sizeof(positions);
    GL().glGenBuffers(1, &vbo);
    GL().glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GL().glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

    // Set up the vertex layout:
    GLsizeiptr stride = 2 * sizeof(positions[0]);
    GL().glEnableVertexAttribArray(PositionSlot);
    GL().glVertexAttribPointer(PositionSlot, 2, GL_SHORT, GL_FALSE, stride, 0);

    return vao;
#endif
    return 0;
}
