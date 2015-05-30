/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#ifndef FLUID_HPP
#define FLUID_HPP

#include <QOpenGLFunctions> // for QOpenGLFunctions definition

typedef struct Surface_ {
    GLuint FboHandle;
    GLuint TextureHandle;
    GLuint RenderBufferHandle;
    int NumComponents;
} Surface;

typedef struct Slab_ {
    Surface Ping;
    Surface Pong;
} Slab;

typedef struct Vector2_ {
    int X;
    int Y;
} Vector2;

#define CellSize (1.25f)

static const float AmbientTemperature = 0.0f;
static const float ImpulseTemperature = 10.0f;
static const float ImpulseDensity = 1.0f;
static const int NumJacobiIterations = 40;
static const float TimeStep = 0.125f;
static const float SmokeBuoyancy = 1.0f;
static const float SmokeWeight = 0.05f;
static const float GradientScale = 1.125f / CellSize;
static const float TemperatureDissipation = 0.99f;
static const float VelocityDissipation = 0.99f;
static const float DensityDissipation = 0.9999f;

static const int PositionSlot = 0;

GLuint CreateProgram(QObject* parent, const char* fsKey);
Surface CreateSurface(GLsizei width, GLsizei height, int numComponents);
Slab CreateSlab(GLsizei width, GLsizei height, int numComponents);
void DestroySlab(Slab slab);
void DestroySurface(Surface surf);
void CreateObstacles(Surface dest, int width, int height, int xpos, int ypos,
                     GLuint program, QOpenGLBuffer* border, QOpenGLBuffer* circle);
void InitSlabOps(QObject* parent);
void SwapSurfaces(Slab* slab);
void ClearSurface(Surface s, float value);
void Advect(Surface velocity, Surface source, Surface obstacles, Surface dest,
            float dissipation, int gridWidth, int gridHeight);
void Jacobi(Surface pressure, Surface divergence, Surface obstacles, Surface dest);
void SubtractGradient(Surface velocity, Surface pressure, Surface obstacles, Surface dest);
void ComputeDivergence(Surface velocity, Surface obstacles, Surface dest);
void ApplyImpulse(Surface dest, Vector2 position, float value, float splatRadius);
void ApplyBuoyancy(Surface velocity, Surface temperature, Surface density, Surface dest);

void FluidInit(QObject* parent);
void FluidResize(int width, int height);
void FluidUpdate(unsigned int elapsedMicroseconds);
void FluidRender(GLuint windowFbo, int width, int height);
void FluidSetCirclePosition(int xpos, int ypos, int width, int height);
void FluidUninit();
void FluidCheckCondition(bool success, const char* errorMsg);

QOpenGLFunctions& GL();

#endif  // FLUID_HPP
