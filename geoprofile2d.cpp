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

void GeoProfile2D::getUniqueVSoilsIDs(QList<int> &vsoilIds)
{
    vsoilIds.clear();
    for(int i=0; i<m_areas->count(); i++){
        int id = m_areas->at(i).vsoilId;
        if (!vsoilIds.contains(id)) vsoilIds.append(id);
    }
}

void GeoProfile2D::optimize()
{
    QList<sArea> optimizedList;
    int start = 0;
    int cid = -1;
    for(int i=0; i<m_areas->count();i++){
        if(i==0){
            start = m_areas->at(i).start;
            cid = m_areas->at(i).vsoilId;
        }
        if(cid != m_areas->at(i).vsoilId || i==m_areas->count()-1){
            sArea a;
            a.start = start;
            if(i==m_areas->count()-1)
                a.end = m_areas->at(i).end;
            else
                a.end = m_areas->at(i).start;
            a.vsoilId = cid;
            optimizedList.append(a);
            cid = m_areas->at(i).vsoilId;
            start = a.end;
        }
    }
    m_areas->clear();
    foreach(sArea a , optimizedList){
       m_areas->append(a);
    }

    /*
    QList<sArea> optimizedAreas;
    int cId = -1;
    int startX = 0;

    for(int i=0; i<m_areas->count(); i++){
        if(cId == -1){ //first entry
            cId = m_areas->at(i).vsoilId;
        }
        //if the id changed or it is the last entry, add this area to the optimized list
        if((cId != m_areas->at(i).vsoilId) || (i==m_areas->count()-1)){
            sArea a;
            a.vsoilId = cId;
            a.start = startX;
            if(i == m_areas->count()-1){ //if at end use the end of the area
                a.end = m_areas->at(i).end;
            }
            else{ //if not use the start of the area
                a.end = m_areas->at(i).start;
            }
            optimizedAreas.append(a);
            startX = m_areas->at(i).start;
            cId = m_areas->at(i).vsoilId;
        }
    }
    m_areas->clear();
    foreach(sArea a , optimizedAreas){
       m_areas->append(a);
    }*/
}
