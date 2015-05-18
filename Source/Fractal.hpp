#ifndef FRACTAL_HPP
#define FRACTAL_HPP

#include <QObject>
#include <QPointF>

class CBuffer;

#define FRACTAL_ELEM_1_BYTE 1

class CFractal
{
public:
    CFractal();
    ~CFractal();

    bool GenerateFractal(int width, int height, unsigned char* data);
    void SetAnimated(bool animated);
    void SetSeedPoint(QPointF position);
    void StopGenerate();

private:
    QPointF m_seed;
    bool m_animated;
    bool m_stop;
};

#endif // FRACTAL_HPP
