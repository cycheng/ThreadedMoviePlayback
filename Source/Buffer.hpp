#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <memory>

class CBuffer
{
public:
    explicit CBuffer();
    ~CBuffer();

    void SetTextureSize(int width, int height);

private:
    int m_width;
    int m_height;
    std::unique_ptr<unsigned char[]> m_buffer;
};

#endif // BUFFER_HPP
