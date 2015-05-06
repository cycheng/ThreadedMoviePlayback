#ifndef FRACTAL_HPP
#define FRACTAL_HPP

#include <QObject>
#include <QPointF>

class CFractal
{
public:
    CFractal();
    ~CFractal();

    bool GenerateFractal();
    void SetAnimated(bool animated);
    void SetSeedPoint(QPointF position);

private:
    QPointF m_seed;
    bool m_animated;
};

#endif // FRACTAL_HPP
