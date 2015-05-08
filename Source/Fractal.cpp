#include "Stdafx.hpp"
#include "Fractal.hpp"
#include "Buffer.hpp"
#include <QTime>
#include <cmath>

namespace
{

    template <int N>
    unsigned char juliaSet(float x, float y, const float seedX, const float seedY)
    {
        int i;

        x = 3.0f * (x - 0.5f);
        y = 2.0f * (y - 0.5f);

        for (i = 0; i < N; i++)
        {
            float tx = (x * x - y * y) + seedX;
            float ty = (y * x + x * y) + seedY;

            if ((tx * tx + ty * ty) > 4.0)
            {
                break;
            }

            x = tx;
            y = ty;
        }

        return i == N ? 0 : i;
    }

}

CFractal::CFractal(): m_seed(0.0f, 0.0f), m_animated(true)
{
}

CFractal::~CFractal()
{
}

bool CFractal::GenerateFractal(CBuffer& buffer)
{
    const int width = buffer.GetWidth();
    const int height = buffer.GetHeight();
    unsigned char* ptr = buffer.GetPtr();

    if (m_animated)
    {
        float t = QTime::currentTime().msecsSinceStartOfDay() / 5000.0;
        m_seed.rx() = (std::sin(std::cos(t / 10.0f) * 10.0f) + std::cos(t * 2.0f) / 4.0f + std::sin(t * 3.0f) / 6.0f) * 0.8f;
        m_seed.ry() = (std::cos(std::sin(t / 10.0f) * 10.0f) + std::sin(t * 2.0f) / 4.0f + std::cos(t * 3.0f) / 6.0f) * 0.8f;
    }

    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            int value = juliaSet<256>(i / static_cast<float>(width), j / static_cast<float>(height), m_seed.rx(), m_seed.ry());
            // TODO: *(static_cast<unsigned char*>(buffer) + i + j * width) = value;
            *(ptr + i + j * width) = value;
        }
    }

    return true;
}

void CFractal::SetAnimated(bool animated)
{
    m_animated = animated;
}

void CFractal::SetSeedPoint(QPointF seed)
{
    m_seed = seed;
}

