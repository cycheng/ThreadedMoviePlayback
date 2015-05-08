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

    bool GenerateFractal(CBuffer& buffer);
    void SetAnimated(bool animated);
    void SetSeedPoint(QPointF position);

private:
    QPointF m_seed;
    bool m_animated;
};

#endif // FRACTAL_HPP
