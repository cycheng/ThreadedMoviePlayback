#ifndef FLUIDFX_HPP
#define FLUIDFX_HPP

class QObject;
class CGLWidget;

class CEffect
{
public:
    CEffect();
    virtual ~CEffect();
    virtual void InitEffect(CGLWidget* parent);
    virtual bool WindowResize(int width, int height);

    virtual void Enable();
    virtual void Disable();

    void Update();
    void Render();

protected:
    virtual void DoUpdate() = 0;
    virtual void DoRender() = 0;

    CGLWidget* m_glwidget;
    bool m_enabled;
    QOpenGLShaderProgram m_program;
    int m_width, m_height;
};

#include "TextureObject.hpp"

class CFractalFX : public CEffect
{
public:
    CFractalFX();
    ~CFractalFX();
    void InitEffect(CGLWidget* parent) override;
    bool WindowResize(int width, int height) override;

    CVideoTexture* GetVideoTexture();
    CFractalTexture* GetFractalTexture();
    void SetAlpha(float alpha);

    enum RENDER_TARGET { RT_TEXTIRE = 1, RT_FRAMEBUFFER };
    void ChangeRenderTarget(RENDER_TARGET rt);
    GLuint GetTextureTarget();

private:
    void DoUpdate() override;
    void DoRender() override;

    CVideoTexture m_videoTex;
    CFractalTexture m_fractalTex;

    /* uniform of fragment shader */
    int m_fractalLoc;
    int m_ffmpegLoc;
    int m_alphaLoc;
    float m_alpha;
};

class CFluidFX: public CEffect
{
public:
    void InitEffect(CGLWidget* parent) override;
    bool WindowResize(int width, int height) override;

private:
    void DoUpdate() override;
    void DoRender() override;
};

/*
class CPageCurlFX : public CEffect {
public:
    void BindTextureTarget();
};
*/

#endif // FLUIDFX_HPP
