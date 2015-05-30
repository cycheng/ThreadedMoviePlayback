#ifndef EFFECT_HPP
#define EFFECT_HPP

class QObject;

class CEffect
{
public:
    CEffect();
    virtual ~CEffect();
    virtual void InitEffect(QObject* parent);
    virtual bool WindowResize(int width, int height);

    virtual void Enable();
    virtual void Disable();

    void Update(int elapsedMs);
    void Render();
    void SetRenderTarget(GLuint rt);
    bool IsEnabled() const;

protected:
    virtual void DoUpdate(int elapsedMs) = 0;
    virtual void DoRender() = 0;

    QObject* m_parent;
    bool m_enabled;
    QOpenGLShaderProgram m_program;
    int m_width, m_height;
    GLuint m_renderTarget;
};

class CVideoTexture;
class CFractalTexture;

class CMoviePlayback: public CEffect
{
public:
    CMoviePlayback();
    void InitEffect(QObject* parent) override;
    void Enable() override;
    void Disable() override;
    void BindTexture(CVideoTexture* video);

private:
    void DoUpdate(int elapsedMs) override;
    void DoRender() override;

    CVideoTexture* m_videoTex;
    // for control framerate
    float m_time;

    /* uniform of fragment shader */
    int m_ffmpegLoc;
};

class CFractalFX: public CEffect
{
public:
    CFractalFX();
    ~CFractalFX();
    void InitEffect(QObject* parent) override;
    void Enable() override;
    void Disable() override;
    void BindTexture(CVideoTexture* video, CFractalTexture* fractal);
    void SetAlpha(float alpha);

private:
    void DoUpdate(int elapsedMs) override;
    void DoRender() override;

    CVideoTexture* m_videoTex;
    CFractalTexture* m_fractalTex;
    // for control framerate
    float m_time;

    /* uniform of fragment shader */
    int m_fractalLoc;
    int m_ffmpegLoc;
    int m_alphaLoc;
    float m_alpha;
};

class CFluidFX: public CEffect
{
public:
    CFluidFX();
    virtual ~CFluidFX();
    void InitEffect(QObject* parent) override;
    bool WindowResize(int width, int height) override;
    void SetMousePosition(int xpos, int ypos);
    void SetSizeLimit(int maxwidth, int maxheight);
    float AdjustX() const;
    float AdjustY() const;
    int RealWidth() const;
    int RealHeight() const;

private:
    void DoUpdate(int elapsedMs) override;
    void DoRender() override;

    int m_widthLimit;
    int m_heightLimit;
    float m_mouseX;
    float m_mouseY;
};

class CPageCurlFX: public CEffect {
public:
    CPageCurlFX();

    void InitEffect(QObject* parent) override;
    void SetInputTextureId(GLuint texid);
    void SetAnimated(bool animated);

private:
    void DoUpdate(int elapsedMs) override;
    void DoRender() override;

    // control page curl rate
    float m_time;
    bool m_animated;

    /** Location of shader attributes and uniforms */
    int m_vertexLoc;
    int m_sourceTexLoc;
    int m_targetTexLoc;
    int m_timeLoc;

    // Previous effect's result
    GLuint m_textureId;
};


#endif // EFFECT_HPP
