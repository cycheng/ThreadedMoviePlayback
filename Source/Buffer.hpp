#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <memory>

#define BGRA_4_BYTES 4

class CBuffer
{
public:
    typedef std::unique_ptr<unsigned char[]> smart_ptr;

    explicit CBuffer();
    virtual ~CBuffer();

    virtual unsigned char* GetWorkingBuffer() = 0;
    virtual unsigned char* GetStableBuffer() = 0;

    //void InitWorkingBuffer(const unsigned char* data, size_t size);
    virtual void InitWorkingBufferWithZero() = 0;

    void SetTextureSize(int width, int height, int pixelSize = BGRA_4_BYTES);

    //unsigned char* GetPtr() const;
    int GetSize() const;
    int GetRowSize() const;
    int GetWidth() const;
    int GetHeight() const;

protected:
    void CheckSize(size_t size);

private:
    virtual void CreateResource(size_t newSize) = 0;

    int m_width;
    int m_height;
    int m_pixelSize;
};

class CSingleBuffer : public CBuffer
{
public:
    unsigned char* GetWorkingBuffer() override;
    unsigned char* GetStableBuffer() override;

    void InitWorkingBufferWithZero() override;
private:
    void CreateResource(size_t newSize) override;

    smart_ptr m_buffer;
};

class CThreadBuffer : public CBuffer
{
public:
    virtual void Wakeup() = 0;

};

class CTripleBuffer : public CThreadBuffer
{
public:
    CTripleBuffer();

    unsigned char* GetWorkingBuffer() override;
    unsigned char* GetStableBuffer() override;
    void InitWorkingBufferWithZero() override;
    void Wakeup() override;
private:
    void CreateResource(size_t newSize) override;
    //void SwapBuffer(smart_ptr& buf1, smart_ptr& buf2);

    QMutex m_mutex;
    QWaitCondition m_emptySignal;
    bool m_workingCopyEmpty;

    smart_ptr m_working;
    smart_ptr m_stable;
    smart_ptr m_workingCopy;
};

#endif // BUFFER_HPP
