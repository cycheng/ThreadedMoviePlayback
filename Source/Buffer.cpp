#include "Stdafx.hpp"
#include "Buffer.hpp"

CBuffer::CBuffer(): m_width(-1), m_height(-1)
{
}

CBuffer::~CBuffer()
{
}

void CBuffer::SetTextureSize(int width, int height)
{
    m_width = width;
    m_height = height;
}