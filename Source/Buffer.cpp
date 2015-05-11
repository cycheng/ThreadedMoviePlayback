#include "Stdafx.hpp"
#include "Buffer.hpp"
#include <sstream>

CBuffer::CBuffer(): m_width(-1), m_height(-1)
{
}

CBuffer::~CBuffer()
{
}

//void CBuffer::InitWorkingBuffer(const unsigned char* data, size_t size)
//{
//    CheckSize(size);
//
//    unsigned char* ptr = //GetWorkingBuffer(); // m_working.get();
//    memcpy(ptr, data, GetSize());
//}

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

void CBuffer::SetTextureSize(int width, int height, int pixelSize)
{
    const size_t newSize = width * height * pixelSize;
    CreateResource(newSize);

    m_width = width;
    m_height = height;
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

void CSingleBuffer::CreateResource(size_t newSize)
{
    if (newSize != GetSize())
    {
        m_buffer.reset(new unsigned char[newSize]);
    }
}

unsigned char* CSingleBuffer::GetWorkingBuffer()
{
    return m_buffer.get();
}

unsigned char* CSingleBuffer::GetStableBuffer()
{
    return m_buffer.get();
}

void CSingleBuffer::InitWorkingBufferWithZero()
{
    unsigned char* ptr = m_buffer.get();//GetWorkingBuffer(); // m_working.get();
    memset(ptr, 0, GetSize());
}

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

unsigned char* CTripleBuffer::GetWorkingBuffer()
{
    QMutexLocker locker(&m_mutex);

    if (! m_workingCopyEmpty)
    {
        m_emptySignal.wait(&m_mutex);
    }

    m_working.swap(m_workingCopy);
    //SwapBuffer(m_working, m_workingCopy);
    m_workingCopyEmpty = false;

    return m_working.get();
}


unsigned char* CTripleBuffer::GetStableBuffer()
{
    QMutexLocker locker(&m_mutex);

    if (! m_workingCopyEmpty)
    {
        //SwapBuffer(m_stable, m_workingCopy);
        m_stable.swap(m_workingCopy);
        m_workingCopyEmpty = true;

        locker.unlock();
        m_emptySignal.wakeOne();
    }

    return m_stable.get();
}

void CTripleBuffer::InitWorkingBufferWithZero()
{
    unsigned char* ptr = m_working.get();
    memset(ptr, 0, GetSize());
    m_workingCopyEmpty = false;
}

void CTripleBuffer::Wakeup()
{
    QMutexLocker locker(&m_mutex);
    m_workingCopyEmpty = true;
    m_emptySignal.wakeOne();
}
