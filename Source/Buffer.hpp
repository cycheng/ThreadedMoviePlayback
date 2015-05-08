#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <memory>

#define BGRA_4_BYTES 4

class CBuffer
{
public:
    explicit CBuffer();
    ~CBuffer();

    void SetTextureSize(int width, int height, int pixelSize = BGRA_4_BYTES);
    unsigned char* GetPtr() const;
    int GetSize() const;
    int GetRowSize() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    int m_width;
    int m_height;
    int m_pixelSize;
    std::unique_ptr<unsigned char[]> m_buffer;
};

#endif // BUFFER_HPP
