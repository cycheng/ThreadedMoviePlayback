#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <memory>

typedef std::unique_ptr<unsigned char[]> u_data_ptr;

class CBuffer
{
public:
    explicit CBuffer();
    virtual ~CBuffer();

    virtual unsigned char* GetWorkingBuffer() const = 0;
    virtual unsigned char* GetStableBuffer() const = 0;
    virtual unsigned char* GetIntermediateBuffer() const = 0;

    virtual void InitIntermediateBufferWithZero();
    virtual void InitIntermediateBuffer(const unsigned char* data, size_t size);

    void SetTextureSize(int width, int height);
    void SetPixelSize(int pixelSize);

    int GetSize() const;
    int GetRowSize() const;
    int GetWidth() const;
    int GetHeight() const;
    int GetPixelSize() const;

protected:
    void CheckSize(size_t size);

private:
    virtual void CreateResource(size_t newSize) = 0;

    int m_width;
    int m_height;
    int m_pixelSize;
};

class CSingleBuffer: public CBuffer
{
public:
    unsigned char* GetWorkingBuffer() const override;
    unsigned char* GetStableBuffer() const override;
    unsigned char* GetIntermediateBuffer() const override;

private:
    void CreateResource(size_t newSize) override;

    u_data_ptr m_buffer;
};

class CWorkerBuffer: public CBuffer
{
public:
    virtual bool CanWeSwapWorkingBuffer() = 0;
    virtual bool CanWeSwapStableBuffer() = 0;
    virtual void SwapWorkingBuffer() = 0;
    virtual void SwapStableBuffer() = 0;
    virtual void SetWorkingBufferFull() = 0;
    virtual void SetWorkingBufferEmpty() = 0;
    virtual void InitAllInternalBuffers(const unsigned char* data, size_t size) = 0;
};

class CTripleBuffer: public CWorkerBuffer
{
public:
    CTripleBuffer();
    ~CTripleBuffer();

    unsigned char* GetWorkingBuffer() const override;
    unsigned char* GetStableBuffer() const override;
    unsigned char* GetIntermediateBuffer() const override;

    void InitIntermediateBufferWithZero() override;
    void InitIntermediateBuffer(const unsigned char* data, size_t size) override;

    bool CanWeSwapWorkingBuffer() override;
    bool CanWeSwapStableBuffer() override;
    void SwapWorkingBuffer() override;
    void SwapStableBuffer() override;
    void SetWorkingBufferFull() override;
    void SetWorkingBufferEmpty() override;
    void InitAllInternalBuffers(const unsigned char* data, size_t size) override;

private:
    void CreateResource(size_t newSize) override;

    bool m_workingCopyEmpty;

    u_data_ptr m_working;
    u_data_ptr m_stable;
    u_data_ptr m_workingCopy;
};

class CDoubleBuffer: public CWorkerBuffer
{
public:
    CDoubleBuffer();
    ~CDoubleBuffer();

    unsigned char* GetWorkingBuffer() const override;
    unsigned char* GetStableBuffer() const override;
    unsigned char* GetIntermediateBuffer() const override;

    void InitIntermediateBufferWithZero() override;
    void InitIntermediateBuffer(const unsigned char* data, size_t size) override;

    bool CanWeSwapWorkingBuffer() override;
    bool CanWeSwapStableBuffer() override;
    void SwapWorkingBuffer() override;
    void SwapStableBuffer() override;
    void SetWorkingBufferFull() override;
    void SetWorkingBufferEmpty() override;
    void InitAllInternalBuffers(const unsigned char* data, size_t size) override;

private:
    void CreateResource(size_t newSize) override;

    bool m_stableEmpty;
    bool m_workFull;

    u_data_ptr m_working;
    u_data_ptr m_stable;
};

/** Helper functions
 */
int GetGLPixelSize(GLenum glImgFmt);

#endif // BUFFER_HPP
