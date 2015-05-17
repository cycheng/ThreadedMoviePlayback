#include "Stdafx.hpp"
#include "Buffer.hpp"
#include <sstream>
#include <assert.h>

CBuffer::CBuffer() : m_width(-1), m_height(-1), m_pixelSize(0)
{
}

CBuffer::~CBuffer()
{
}

void CBuffer::CheckSize(size_t size)
{
    if (size != GetSize())
    {
        std::ostringstream err;
        err << "InitWorkingBuffer size mismatch = " << size
            << ", expected = " << GetSize() << std::endl;
        throw std::runtime_error(err.str());
    }
}

void CBuffer::SetTextureSize(int width, int height)
{
    assert(m_pixelSize);

    const size_t newSize = width * height * m_pixelSize;

    CreateResource(newSize);

    m_width = width;
    m_height = height;
}

void CBuffer::SetPixelSize(int pixelSize)
{
    m_pixelSize = pixelSize;
}

int CBuffer::GetSize() const
{
    return m_width * m_height * m_pixelSize;
}

int CBuffer::GetRowSize() const
{
    return m_width * m_pixelSize;
}

int CBuffer::GetWidth() const
{
    return m_width;
}

int CBuffer::GetHeight() const
{
    return m_height;
}

int CBuffer::GetPixelSize() const
{
    return m_pixelSize;
}

// -----------------------------------------------------------------------------
// CSingleBuffer Functions
// -----------------------------------------------------------------------------
void CSingleBuffer::CreateResource(size_t newSize)
{
    if (newSize != GetSize())
    {
        m_buffer.reset(new unsigned char[newSize]);
    }
}

unsigned char* CSingleBuffer::GetWorkingBuffer() const
{
    return m_buffer.get();
}

unsigned char* CSingleBuffer::GetStableBuffer() const
{
    return m_buffer.get();
}

void CSingleBuffer::InitResultBufferWithZero()
{
    unsigned char* ptr = m_buffer.get();
    memset(ptr, 0, GetSize());
}

// -----------------------------------------------------------------------------
// CTripleBuffer Functions
// -----------------------------------------------------------------------------
CTripleBuffer::CTripleBuffer() : m_workingCopyEmpty(true)
{
}

void CTripleBuffer::CreateResource(size_t newSize)
{
    if (newSize != GetSize())
    {
        m_working.reset(new unsigned char[newSize]);
        m_stable.reset(new unsigned char[newSize]);
        m_workingCopy.reset(new unsigned char[newSize]);
    }
}

void CTripleBuffer::InitResultBufferWithZero()
{
    unsigned char* ptr = m_working.get();
    memset(ptr, 0, GetSize());
    m_workingCopyEmpty = false;
}

unsigned char* CTripleBuffer::GetWorkingBuffer() const
{
    return m_working.get();
}

unsigned char* CTripleBuffer::GetStableBuffer() const
{
    return m_stable.get();
}

bool CTripleBuffer::CanWeSwapWorkingBuffer()
{
    return m_workingCopyEmpty;
}

bool CTripleBuffer::CanWeSwapStableBuffer()
{
    return ! m_workingCopyEmpty;
}

void CTripleBuffer::SwapWorkingBuffer()
{
    m_working.swap(m_workingCopy);
    m_workingCopyEmpty = false;
}

void CTripleBuffer::SwapStableBuffer()
{
    m_stable.swap(m_workingCopy);
    m_workingCopyEmpty = true;
}

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------
int GetGLPixelSize(GLenum glImgFmt) {
    switch (glImgFmt) {
    case GL_RGBA:
    case GL_BGRA:
        return 4;

    case GL_RED:
    case GL_ALPHA:
    case GL_LUMINANCE:
        return 1;

    default:
        // TODO
        assert(false);
        return 0;
    }
}
