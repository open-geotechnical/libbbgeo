#include "vsoil.h"

/*
    A VSoil object contains information of soil layers that are stacked
    on top of each other (like in a borehole). VSoil stands for vertical soil
    The main information is in soillayers which looks like
                  0     1         2
    Soil layer = zmax, zmin, soillayer_id

    The source shows where the information came from (for example `auto-generated'
    if a correlation is done)
 */
VSoil::VSoil(QObject *parent) :
    QObject(parent)
{
    m_id = -1;
    m_source = "";
    m_x = 0.;
    m_y = 0.;
    m_lat = 0.;
    m_lng = 0.;
    m_soilLayers = new QList<VSoilLayer>();
    m_dataChanged = FALSE;
    m_name = "";
}

VSoil::~VSoil()
{
    m_soilLayers->clear();
    m_soilLayers = NULL;
}

/*
    Reads a blob from the database with the layout
    top;bottom;soillayer_id
 */
void VSoil::blobToData(QString data)
{
    QStringList lines = data.split("\n");
    for(int i=0; i<lines.count(); i++){
        QString line = lines[i];
        if(line.trimmed().count() > 0){
            QStringList args = line.split(';');
            VSoilLayer vs;
            vs.zmax = args[0].toDouble();
            vs.zmin = args[1].toDouble();
            vs.soiltype_id = args[2].toInt();
            m_soilLayers->append(vs);
        }
    }
}

/*
    Returns a string like;
    top;bottom;soillayer_id
 */
QByteArray VSoil::dataAsQByteArray(){
    QByteArray result;
    for(int i=0; i<m_soilLayers->count(); i++){
        result.append(QString("%1;%2;%3\n")
                      .arg(m_soilLayers->at(i).zmax, 2, 'f')
                      .arg(m_soilLayers->at(i).zmin, 2, 'f')
                      .arg(m_soilLayers->at(i).soiltype_id, 'd'));
    }
    return result;
}

double VSoil::zMin(){
    if(m_soilLayers->count()>0){
        return m_soilLayers->at(m_soilLayers->count()-1).zmin;
    }
    return 9999.;
    //TODO: raise exception
}

double VSoil::zMax(){
    if(m_soilLayers->count()>0){
        return m_soilLayers->at(0).zmax;
    }
    return -9999.;
    //TODO: raise exception
}

/*
    Optimization makes sure that two (or more) layers that are next to each
    other are rebuild into one layer.
 */
void VSoil::optimize(){
    QList<VSoilLayer> newList;
    double z1 = 0.;
    int id = -1;

    for(int i=0; i<m_soilLayers->count();i++){
        if(i==0){ //first soillayer so set the first limit to the top of this layer
            z1 = m_soilLayers->at(i).zmax;
            id = m_soilLayers->at(i).soiltype_id;
        }
        else if(i==m_soilLayers->count()-1){ //last layer, add this layer
            VSoilLayer vs;
            vs.zmax = z1;
            vs.zmin = m_soilLayers->at(i).zmin;
            vs.soiltype_id = id;
            newList.append(vs);
        }else if(id!=m_soilLayers->at(i).soiltype_id){ //new id, so the layer needs to be added
            VSoilLayer vs;
            vs.zmax = z1;
            vs.zmin = m_soilLayers->at(i).zmax;
            vs.soiltype_id = id;
            newList.append(vs);
            z1 = m_soilLayers->at(i).zmax; //we have a new start limit
            id = m_soilLayers->at(i).soiltype_id; //and a new id to look for
        }
    }
    //done, now copy the list to the original list
    m_soilLayers->clear();
    for(int i=0; i<newList.count(); i++){
        m_soilLayers->append(newList[i]);
    }
}

void VSoil::addSoilLayer(double zmax, double zmin, int id)
{
    VSoilLayer sl;
    sl.soiltype_id = id;
    sl.zmax = zmax;
    sl.zmin = zmin;
    m_soilLayers->append(sl);
}
