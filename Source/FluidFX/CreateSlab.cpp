/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#include "Stdafx.hpp"
#include "Fluid.hpp"
#include <cassert>

Slab CreateSlab(GLsizei width, GLsizei height, int numComponents)
{
    Slab slab;
    slab.Ping = CreateSurface(width, height, numComponents);
    slab.Pong = CreateSurface(width, height, numComponents);
    return slab;
}

Surface CreateSurface(GLsizei width, GLsizei height, int numComponents)
{
    GLuint fboHandle;
    GL().glGenFramebuffers(1, &fboHandle);
    GL().glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

    GLuint textureHandle;
    GL().glGenTextures(1, &textureHandle);
    GL().glBindTexture(GL_TEXTURE_2D, textureHandle);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL().glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int UseHalfFloats = 1;
    if (UseHalfFloats) {
        switch (numComponents) {
            case 1: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_HALF_FLOAT, 0); break;
            case 2: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_HALF_FLOAT, 0); break;
            case 3: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0); break;
            case 4: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, 0); break;
            default: assert(false && "Illegal slab format.");
        }
    } else {
        switch (numComponents) {
            case 1: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0); break;
            case 2: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, 0); break;
            case 3: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); break;
            case 4: GL().glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0); break;
            default: assert(false && "Illegal slab format.");
        }
    }

    FluidCheckCondition(GL_NO_ERROR == GL().glGetError(), "Unable to create normals texture");

    GLuint colorbuffer;
    GL().glGenRenderbuffers(1, &colorbuffer);
    GL().glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    GL().glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);
    FluidCheckCondition(GL_NO_ERROR == GL().glGetError(), "Unable to attach color buffer");

    FluidCheckCondition(GL_FRAMEBUFFER_COMPLETE == GL().glCheckFramebufferStatus(GL_FRAMEBUFFER), "Unable to create FBO.");
    Surface surface = { fboHandle, textureHandle, numComponents };

    GL().glClearColor(0, 0, 0, 0);
    GL().glClear(GL_COLOR_BUFFER_BIT);
    GL().glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return surface;
}
