#include "Stdafx.hpp"
#include "Buffer.hpp"

CBuffer::CBuffer(): m_width(-1), m_height(-1)
{
}

CBuffer::~CBuffer()
{
}

void CBuffer::SetTextureSize(int width, int height, int pixelSize)
{
    const size_t newSize = width * height * pixelSize;

    if (newSize != GetSize()) {
        m_buffer.reset(new unsigned char[newSize]);
    }
    m_width = width;
    m_height = height;
    m_pixelSize = pixelSize;
}

unsigned char* CBuffer::GetPtr() const {
    return m_buffer.get();
}

int CBuffer::GetSize() const {
    return m_width * m_height * m_pixelSize;
}

int CBuffer::GetRowSize() const {
    return m_width * m_pixelSize;
}

int CBuffer::GetWidth() const {
    return m_width;
}

int CBuffer::GetHeight() const {
    return m_height;
}
