#include "geoprofile2d.h"
#include "vsoil.h"

GeoProfile2D::GeoProfile2D(QObject *parent) :
    QObject(parent)
{
    m_areas = new QList<sArea>();
    m_points = new QList<QPointF>();
    m_soilTypeIds = new QList<int>();
}

GeoProfile2D::~GeoProfile2D()
{
    m_areas->clear(); //TODO: check if this has to be a pointer
    delete m_areas;
    m_points->clear(); //TODO: check if this has to be a pointer
    delete m_points;
    m_soilTypeIds->clear();
    delete m_soilTypeIds;
}

double GeoProfile2D::lMax()
{
    if(m_areas->count()>0)
        return m_areas->at(m_areas->count()-1).end;
    else
        return 0;
}

void GeoProfile2D::addSoilTypeIDs(VSoil *vs)
{
    for(int i=0; i<vs->getSoilLayers()->count(); i++){
        int id = vs->getSoilLayers()->at(i).soiltype_id;
        bool add = true;
        for(int j=0; j<m_soilTypeIds->count(); j++){
            if(m_soilTypeIds->at(j) == id){
                add = false;
                break;
            }
        }
        if(add) m_soilTypeIds->append(id);
    }
}
