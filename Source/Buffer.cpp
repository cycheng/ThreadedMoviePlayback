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

void CBuffer::InitIntermediateBufferWithZero()
{
    unsigned char* ptr = GetIntermediateBuffer();
    memset(ptr, 0, GetSize());
}

void CBuffer::InitIntermediateBuffer(const unsigned char* data, size_t size)
{
    CheckSize(size);
    memcpy(GetIntermediateBuffer(), data, size);
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

unsigned char* CSingleBuffer::GetIntermediateBuffer() const
{
    return m_buffer.get();
}

// -----------------------------------------------------------------------------
// CTripleBuffer Functions
// -----------------------------------------------------------------------------
CTripleBuffer::CTripleBuffer() : m_workingCopyEmpty(true)
{
}

CTripleBuffer::~CTripleBuffer()
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

void CTripleBuffer::InitIntermediateBufferWithZero()
{
    memset(m_workingCopy.get(), 0, GetSize());
    m_workingCopyEmpty = false;
}

void CTripleBuffer::InitIntermediateBuffer(const unsigned char* data, size_t size)
{
    CheckSize(size);
    memcpy(m_workingCopy.get(), data, size);
    m_workingCopyEmpty = false;
}

void CTripleBuffer::InitAllInternalBuffers(const unsigned char* data, size_t size)
{
    CheckSize(size);
    memcpy(m_stable.get(), data, size);
    memcpy(m_workingCopy.get(), data, size);
    memcpy(m_working.get(), data, size);
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

unsigned char* CTripleBuffer::GetIntermediateBuffer() const
{
    return m_workingCopy.get();
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

void CTripleBuffer::SetWorkingBufferFull()
{
}

// -----------------------------------------------------------------------------
// CDoubleBuffer Functions
// -----------------------------------------------------------------------------
CDoubleBuffer::CDoubleBuffer(): m_workFull(false)
{
}

CDoubleBuffer::~CDoubleBuffer()
{
}

void CDoubleBuffer::CreateResource(size_t newSize)
{
    if (newSize != GetSize())
    {
        m_working.reset(new unsigned char[newSize]);
        m_stable.reset(new unsigned char[newSize]);
    }
}

void CDoubleBuffer::InitIntermediateBufferWithZero()
{
    memset(m_stable.get(), 0, GetSize());
    m_workFull = false;
}

void CDoubleBuffer::InitIntermediateBuffer(const unsigned char* data, size_t size)
{
    CheckSize(size);
    memcpy(m_stable.get(), data, size);
    m_workFull = false;
}

void CDoubleBuffer::InitAllInternalBuffers(const unsigned char* data, size_t size)
{
    CheckSize(size);
    memcpy(m_working.get(), data, size);
    memcpy(m_stable.get(), data, size);
}

unsigned char* CDoubleBuffer::GetWorkingBuffer() const
{
    return m_working.get();
}

unsigned char* CDoubleBuffer::GetStableBuffer() const
{
    return m_stable.get();
}

unsigned char* CDoubleBuffer::GetIntermediateBuffer() const
{
    return m_working.get();
}

bool CDoubleBuffer::CanWeSwapWorkingBuffer()
{
    return !m_workFull;
}

bool CDoubleBuffer::CanWeSwapStableBuffer()
{
    return m_workFull;
}

void CDoubleBuffer::SwapWorkingBuffer()
{
    /* swap in SwapStableBuffer() */
    m_workFull = false;
}

void CDoubleBuffer::SwapStableBuffer()
{
    m_stable.swap(m_working);
    m_workFull = false;
}

void CDoubleBuffer::SetWorkingBufferFull()
{
    m_workFull = true;
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

