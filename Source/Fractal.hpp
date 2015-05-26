#ifndef FRACTAL_HPP
#define FRACTAL_HPP

#include <QObject>
#include <QPointF>

class CFractal
{
public:
    CFractal();
    virtual ~CFractal();

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
