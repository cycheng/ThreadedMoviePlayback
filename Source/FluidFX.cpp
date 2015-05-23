#include "Stdafx.hpp"
#include "FluidFX.hpp"
#include "FluidFX/Fluid.hpp"

/*  Fluid Simulation Implementation comes from
 *  Philip Rideout
 *  http://prideout.net/blog/?p=58
 */

CFluidFXTexture::CFluidFXTexture(QObject* parent)
{
    FluidInit(parent);
}

CFluidFXTexture::~CFluidFXTexture()
{
}

void CFluidFXTexture::DoUpdate(CBuffer* buffer)
{
    FluidUpdate(0);
}

void CFluidFXTexture::Resize(int width, int height)
{
    if (m_buffer.GetWidth() == width && m_buffer.GetHeight() == height) {
        return;
    }

    if (width > 500) width = 500;
    if (height > 500) height = 500;

    CTextureObject::Resize(width, height);
    FluidResize(width, height);
}

void CFluidFXTexture::Render()
{
    FluidRender(0, m_buffer.GetWidth(), m_buffer.GetHeight());
}
