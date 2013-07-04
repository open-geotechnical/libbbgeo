#ifndef GEOPROFILE2D_H
#define GEOPROFILE2D_H

#include <QObject>
#include <QList>
#include <QPointF>

#include "vsoil.h"
#include "soiltype.h"

struct sArea{
    int start;
    int end;
    int vsoilId;
};

class GeoProfile2D : public QObject
{
    Q_OBJECT
public:
    explicit GeoProfile2D(QObject *parent = 0);
    ~GeoProfile2D();

    QList<sArea> *areas() { return m_areas; }
    QList<QPointF> *points() { return m_points; }
    QList<int> *soilTypeIDs() { return m_soilTypeIds; }

    double lMin() { return 0; }
    double lMax();
    double zMin() { return m_zmin; }
    double zMax() { return m_zmax; }

    void setZMin(double zmin) { m_zmin = zmin; }
    void setZMax(double zmax) { m_zmax = zmax; }

    void addSoilTypeIDs(VSoil *vs);

    void getUniqueVSoilsIDs(QList<int> &vsoilIds);
    void optimize(); //avoids two or more consecutive areas with the same id

private:
    QList<sArea> *m_areas;
    QList<QPointF> *m_points;
    QList<int> *m_soilTypeIds;
    double m_zmin;    
    double m_zmax;
    
signals:
    
public slots:
    
};

#endif // GEOPROFILE2D_H
