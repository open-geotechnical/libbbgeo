#include "latlon.h"
#include "math.h"

LatLon::LatLon()
{
    LatLon(0., 0.);
}

LatLon::LatLon(double lat, double lon)
{
    m_longitude = lon;
    m_latitude = lat;
}

LatLon::LatLon(QPointF p)
{
    m_longitude = p.x();
    m_latitude = p.y();
}

/*
    Converts latitude and longitude values into (Dutch) RD coordinates.
 */
QPointF LatLon::asRDCoords()
{
    double x0 = 155000.;
    double y0 = 463000.;
    double phi0 = 52.15517440;
    double lambda0 = 5.38720621;

    double R[4][5] = {{0., 190094.945, -0.008, -32.391, 0.},
                      {-0.705, -11832.228, 0., -0.608, 0.},
                      {0., -114.211, 0., 0.148, 0.},
                      {0., -2.340, 0., 0., 0.}};
    double S[4][5] = {{0., 0.433, 3638.893, 0., 0.092},
                      {309056.544, -0.032, -157.984, 0., -0.054},
                      {73.077, 0., -6.439, 0., 0.},
                      {59.788, 0., 0., 0., 0.}};

    double dphi = 0.36 * (m_latitude - phi0);
    double dlambda = 0.36 * (m_longitude - lambda0);

    double x = 0.;
    double y = 0.;

    for (int q=0; q<5; q++){
        for (int p=0; p<4; p++){
            x += R[p][q] * pow(dphi, float(p)) * pow(dlambda, float(q));
            y += S[p][q] * pow(dphi, float(p)) * pow(dlambda, float(q));
        }
    }

    x += x0;
    y += y0;
    return QPoint(x, y);
}

/*
    Converts (Dutch) RD coordinates into latitude and longitude values.
 */
void LatLon::fromRDCoords(double x, double y)
{
    double x0 = 155000.;
    double y0 = 463000.;
    double phi0 = 52.15517440;
    double lambda0 = 5.38720621;

    double K[6][5] = {{0., 3235.65389, -.24750, -.06550, 0.},
                      {-.00738, -.00012, 0., 0., 0.},
                      {-32.58297, -.84978, -.01709, -.00039, 0.},
                      {0., 0., 0., 0., 0.},
                      {.00530, 0.00033, 0., 0., 0.},
                      {0., 0., 0., 0., 0.}};
    double L[6][5] = {{0., .01199, 0.00022, 0., 0.},
                      {5260.52916, 105.94684, 2.45656, .05594, .00128},
                      {-.00022, 0., 0., 0., 0.},
                      {-.81885, -.05607, -.00256, 0., 0.},
                      {0., 0., 0., 0., 0.},
                      {.00026, 0., 0., 0., 0.}};

    double dx = (x - x0) * .00001;
    double dy = (y - y0) * .00001;

    double phi = 0.;
    double lam = 0.;

    for(int q=0; q<5; q++){
        for (int p=0; p<6; p++){
            phi += K[p][q] * pow(dx, float(p)) * pow(dy, float(q));
            lam += L[p][q] * pow(dx, float(p)) * pow(dy, float(q));
        }
    }

    m_latitude = phi0 + phi / 3600.;
    m_longitude = lambda0 + lam / 3600.;
}

