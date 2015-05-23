#ifndef FLUIDFX_HPP
#define FLUIDFX_HPP

/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

#include "TextureObject.hpp"

class QObject;

class CFluidFXTexture: public CTextureObject
{
public:
    CFluidFXTexture(QObject* parent);
    ~CFluidFXTexture();

    void DoUpdate(CBuffer* buffer) override;
    void Resize(int width, int height) override;
    void Render();
};


#endif // FLUIDFX_HPP
