#ifndef LATLON_H
#define LATLON_H

#include <QPointF>

class LatLon
{
public:
    explicit LatLon();
    explicit LatLon(double lat, double lon);
    explicit LatLon(QPointF p);

    void setLongitude(double lon) { m_longitude = lon; }
    void setLatitude(double lat) { m_latitude = lat; }
    double getLongitude() { return m_longitude; }
    double getLatitude() { return m_latitude; }

    QPointF asRDCoords();
    void fromRDCoords(double x, double y);

private:
    double m_longitude;
    double m_latitude;
    
};

#endif // LATLON_H
