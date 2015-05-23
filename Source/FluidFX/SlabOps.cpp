/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#include "Stdafx.hpp"
#include "Fluid.hpp"
#include "../GLWidget.hpp"
#include <math.h>

struct ProgramsRec {
    GLuint Advect;
    GLuint Jacobi;
    GLuint SubtractGradient;
    GLuint ComputeDivergence;
    GLuint ApplyImpulse;
    GLuint ApplyBuoyancy;
} Programs;

static void ResetState()
{
    GL().glActiveTexture(GL_TEXTURE2); GL().glBindTexture(GL_TEXTURE_2D, 0);
    GL().glActiveTexture(GL_TEXTURE1); GL().glBindTexture(GL_TEXTURE_2D, 0);
    GL().glActiveTexture(GL_TEXTURE0); GL().glBindTexture(GL_TEXTURE_2D, 0);
    //GL().glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL().glDisable(GL_BLEND);
}

QOpenGLShaderProgram* CreateProgram2(QObject* parent, const char* fsKey);
void InitSlabOps(QObject* parent)
{
    Programs.Advect = CreateProgram(parent, "Advect.frag");
    Programs.Jacobi = CreateProgram(parent, "Jacobi.frag");
    Programs.SubtractGradient = CreateProgram(parent, "SubtractGradient.frag");
    Programs.ComputeDivergence = CreateProgram(parent, "ComputeDivergence.frag");
    Programs.ApplyImpulse = CreateProgram(parent, "Splat.frag");
    Programs.ApplyBuoyancy = CreateProgram(parent, "Buoyancy.frag");
}

void SwapSurfaces(Slab* slab)
{
    Surface temp = slab->Ping;
    slab->Ping = slab->Pong;
    slab->Pong = temp;
}

void ClearSurface(Surface s, float v)
{
    GL().glBindFramebuffer(GL_FRAMEBUFFER, s.FboHandle);
    GL().glClearColor(v, v, v, v);
    GL().glClear(GL_COLOR_BUFFER_BIT);
}

void Advect(Surface velocity, Surface source, Surface obstacles, Surface dest,
            float dissipation, int gridWidth, int gridHeight)
{
    GLuint p = Programs.Advect;
    GL().glUseProgram(p);
    GLint inverseSize = GL().glGetUniformLocation(p, "InverseSize");
    GLint timeStep = GL().glGetUniformLocation(p, "TimeStep");
    GLint dissLoc = GL().glGetUniformLocation(p, "Dissipation");
    GLint sourceTexture = GL().glGetUniformLocation(p, "SourceTexture");
    GLint obstaclesTexture = GL().glGetUniformLocation(p, "Obstacles");

    GL().glUniform2f(inverseSize, 1.0f / gridWidth, 1.0f / gridHeight);
    GL().glUniform1f(timeStep, TimeStep);
    GL().glUniform1f(dissLoc, dissipation);
    GL().glUniform1i(sourceTexture, 1);
    GL().glUniform1i(obstaclesTexture, 2);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, source.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE2);
    GL().glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}

void Jacobi(Surface pressure, Surface divergence, Surface obstacles, Surface dest)
{
    GLuint p = Programs.Jacobi;
    GL().glUseProgram(p);

    GLint alpha = GL().glGetUniformLocation(p, "Alpha");
    GLint inverseBeta = GL().glGetUniformLocation(p, "InverseBeta");
    GLint dSampler = GL().glGetUniformLocation(p, "Divergence");
    GLint oSampler = GL().glGetUniformLocation(p, "Obstacles");

    GL().glUniform1f(alpha, -CellSize * CellSize);
    GL().glUniform1f(inverseBeta, 0.25f);
    GL().glUniform1i(dSampler, 1);
    GL().glUniform1i(oSampler, 2);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, pressure.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, divergence.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE2);
    GL().glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}

void SubtractGradient(Surface velocity, Surface pressure, Surface obstacles, Surface dest)
{
    GLuint p = Programs.SubtractGradient;
    GL().glUseProgram(p);

    GLint gradientScale = GL().glGetUniformLocation(p, "GradientScale");
    GL().glUniform1f(gradientScale, GradientScale);
    GLint halfCell = GL().glGetUniformLocation(p, "HalfInverseCellSize");
    GL().glUniform1f(halfCell, 0.5f / CellSize);
    GLint sampler = GL().glGetUniformLocation(p, "Pressure");
    GL().glUniform1i(sampler, 1);
    sampler = GL().glGetUniformLocation(p, "Obstacles");
    GL().glUniform1i(sampler, 2);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, pressure.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE2);
    GL().glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}

void ComputeDivergence(Surface velocity, Surface obstacles, Surface dest)
{
    GLuint p = Programs.ComputeDivergence;
    GL().glUseProgram(p);

    GLint halfCell = GL().glGetUniformLocation(p, "HalfInverseCellSize");
    GL().glUniform1f(halfCell, 0.5f / CellSize);
    GLint sampler = GL().glGetUniformLocation(p, "Obstacles");
    GL().glUniform1i(sampler, 1);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}

void ApplyImpulse(Surface dest, Vector2 position, float value, float splatRadius)
{
    GLuint p = Programs.ApplyImpulse;
    GL().glUseProgram(p);

    GLint pointLoc = GL().glGetUniformLocation(p, "Point");
    GLint radiusLoc = GL().glGetUniformLocation(p, "Radius");
    GLint fillColorLoc = GL().glGetUniformLocation(p, "FillColor");

    GL().glUniform2f(pointLoc, (float) position.X, (float) position.Y);
    GL().glUniform1f(radiusLoc, splatRadius);
    GL().glUniform3f(fillColorLoc, value, value, value);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glEnable(GL_BLEND);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}

void ApplyBuoyancy(Surface velocity, Surface temperature, Surface density, Surface dest)
{
    GLuint p = Programs.ApplyBuoyancy;
    GL().glUseProgram(p);

    GLint tempSampler = GL().glGetUniformLocation(p, "Temperature");
    GLint inkSampler = GL().glGetUniformLocation(p, "Density");
    GLint ambTemp = GL().glGetUniformLocation(p, "AmbientTemperature");
    GLint timeStep = GL().glGetUniformLocation(p, "TimeStep");
    GLint sigma = GL().glGetUniformLocation(p, "Sigma");
    GLint kappa = GL().glGetUniformLocation(p, "Kappa");

    GL().glUniform1i(tempSampler, 1);
    GL().glUniform1i(inkSampler, 2);
    GL().glUniform1f(ambTemp, AmbientTemperature);
    GL().glUniform1f(timeStep, TimeStep);
    GL().glUniform1f(sigma, SmokeBuoyancy);
    GL().glUniform1f(kappa, SmokeWeight);

    GL().glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
    GL().glActiveTexture(GL_TEXTURE0);
    GL().glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE1);
    GL().glBindTexture(GL_TEXTURE_2D, temperature.TextureHandle);
    GL().glActiveTexture(GL_TEXTURE2);
    GL().glBindTexture(GL_TEXTURE_2D, density.TextureHandle);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    ResetState();
}
