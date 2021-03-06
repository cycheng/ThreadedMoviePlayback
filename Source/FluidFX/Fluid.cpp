/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#include "Stdafx.hpp"
#include "Fluid.hpp"
#include <QOpenglBuffer>

static GLuint QuadVao;
static GLuint VisualizeProgram;
static GLuint FillProgram;
static Slab Velocity, Density, Pressure, Temperature;
static Surface Divergence, Obstacles, HiresObstacles;

static QOpenGLBuffer* BorderObstacleVbo, *CircleObstacleVbo;
static QOpenGLBuffer* ForRenderBorderObstacleVbo, *ForRenderCircleObstacleVbo;

// size parameters
static int GridWidth, GridHeight;
static Vector2 ImpulsePosition;
static float SplatRadius;

void FluidInit(QObject* parent)
{
    InitSlabOps(parent);
    VisualizeProgram = CreateProgram(parent, "Visualize.frag");
    FillProgram = CreateProgram(parent, "Fill.frag");

    QOpenGLBuffer** vboArray[] = {
        &BorderObstacleVbo, &CircleObstacleVbo,
        &ForRenderBorderObstacleVbo, &ForRenderCircleObstacleVbo
    };

    for (QOpenGLBuffer** vbo : vboArray) {
        (*vbo) = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        (*vbo)->create();
        (*vbo)->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }
}

void FluidResize(int width, int height)
{
    if (GridWidth != 0)
    {
        DestroySlab(Velocity);
        DestroySlab(Density);
        DestroySlab(Pressure);
        DestroySlab(Temperature);
        DestroySurface(Divergence);
    }

    // update all size parameters
    GridWidth = width / 2;
    GridHeight = height / 2;
    SplatRadius = ((float)GridWidth / 8.0f);
    ImpulsePosition = { GridWidth / 2, -(int)SplatRadius / 2 };

    int w = GridWidth;
    int h = GridHeight;

    Velocity = CreateSlab(w, h, 2);
    Density = CreateSlab(w, h, 1);
    Pressure = CreateSlab(w, h, 1);
    Temperature = CreateSlab(w, h, 1);
    Divergence = CreateSurface(w, h, 3);

    ClearSurface(Temperature.Ping, AmbientTemperature);
}

void FluidUpdate(unsigned int elapsedMicroseconds)
{
    glViewport(0, 0, GridWidth, GridHeight);

    Advect(Velocity.Ping, Velocity.Ping, Obstacles, Velocity.Pong, VelocityDissipation, GridWidth, GridHeight);
    SwapSurfaces(&Velocity);

    Advect(Velocity.Ping, Temperature.Ping, Obstacles, Temperature.Pong, TemperatureDissipation, GridWidth, GridHeight);
    SwapSurfaces(&Temperature);

    Advect(Velocity.Ping, Density.Ping, Obstacles, Density.Pong, DensityDissipation, GridWidth, GridHeight);
    SwapSurfaces(&Density);

    ApplyBuoyancy(Velocity.Ping, Temperature.Ping, Density.Ping, Velocity.Pong);
    SwapSurfaces(&Velocity);

    ApplyImpulse(Temperature.Ping, ImpulsePosition, ImpulseTemperature, SplatRadius);
    ApplyImpulse(Density.Ping, ImpulsePosition, ImpulseDensity, SplatRadius);

    ComputeDivergence(Velocity.Ping, Obstacles, Divergence);
    ClearSurface(Pressure.Ping, 0);

    for (int i = 0; i < NumJacobiIterations; ++i) {
        Jacobi(Pressure.Ping, Divergence, Obstacles, Pressure.Pong);
        SwapSurfaces(&Pressure);
    }

    SubtractGradient(Velocity.Ping, Pressure.Ping, Obstacles, Velocity.Pong);
    SwapSurfaces(&Velocity);
}

void FluidRender(GLuint windowFbo, int width, int height, bool blueObstacle)
{
    // Bind visualization shader and set up blend state:
    GL().glUseProgram(VisualizeProgram);
    GLint fillColor = GL().glGetUniformLocation(VisualizeProgram, "FillColor");
    GLint scale = GL().glGetUniformLocation(VisualizeProgram, "Scale");
    GL().glEnable(GL_BLEND);

    // Set render target to the backbuffer:
    GL().glViewport(0, 0, width, height);
    GL().glBindFramebuffer(GL_FRAMEBUFFER, windowFbo);

    // Draw ink:
    GL().glBindTexture(GL_TEXTURE_2D, Density.Ping.TextureHandle);
    GL().glUniform3f(fillColor, 1, 1, 1);
    GL().glUniform2f(scale, 1.0f / width, 1.0f / height);
    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw obstacles:
    GL().glBindTexture(GL_TEXTURE_2D, HiresObstacles.TextureHandle);
    if (blueObstacle) {
        GL().glUniform3f(fillColor, 0.125f, 0.4f, 0.75f);
    }
    else {
        GL().glUniform3f(fillColor,1.f, 0.4f, 0.75f);
    }

    GL().glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Disable blending:
    GL().glDisable(GL_BLEND);
}

void FluidUninit()
{
    if (GridWidth != 0)
    {
        DestroySlab(Velocity);
        DestroySlab(Density);
        DestroySlab(Pressure);
        DestroySlab(Temperature);
        DestroySurface(Divergence);
        DestroySurface(Obstacles);
        DestroySurface(HiresObstacles);
    }
    delete BorderObstacleVbo;
    delete CircleObstacleVbo;
    delete ForRenderBorderObstacleVbo;
    delete ForRenderCircleObstacleVbo;
}

void PezHandleMouse(int x, int y, int action)
{
}

void FluidCheckCondition(bool success, const char* errorMsg)
{
    if (! success) {
        throw std::runtime_error(errorMsg);
    }
}

void FluidSetCirclePosition(float xpos, float ypos, int width, int height,
                            float xadjuster, float yadjuster)
{
    int w = width / 2; // i.e. GridWidth;
    int h = height / 2; // i.e. GridHeight;

    ypos = 1.f - ypos;
    CreateObstacles(Obstacles, w, h, xadjuster, yadjuster, xpos, ypos,
                    FillProgram, BorderObstacleVbo, CircleObstacleVbo);

    w = width * 2;
    h = height * 2;
    CreateObstacles(HiresObstacles, w, h, xadjuster, yadjuster, xpos, ypos,
                    FillProgram, ForRenderBorderObstacleVbo, ForRenderCircleObstacleVbo);

    // bind framebuffer back to zero, prevent obstacles framebuffer be cleared
    // accidently
    GL().glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidObstacleResize(float xpos, float ypos, int width, int height,
                         float xadjuster, float yadjuster)
{
    int w = width / 2; // i.e. GridWidth;
    int h = height / 2; // i.e. GridHeight;

    if (GridWidth != 0)
    {
        DestroySurface(Obstacles);
        DestroySurface(HiresObstacles);
    }

    Obstacles = CreateSurface(w, h, 3);
    w = width * 2;
    h = height * 2;
    HiresObstacles = CreateSurface(w, h, 1);
    FluidSetCirclePosition(xpos, ypos, width, height, xadjuster, yadjuster);
}

