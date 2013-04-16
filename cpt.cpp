#include "cpt.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>

#include <QDebug>
#include <QFileInfo>

#include "latlon.h"
#include <cmath>

#define COLVOID 9999

CPT::CPT(QObject *parent) :
    QObject(parent)
{
    m_metaData.id = -1;
    m_metaData.name = "";
    m_metaData.x = 0.;
    m_metaData.y = 0.;
    m_metaData.zmax = 0.;
    m_metaData.zmin = 0.;
    m_metaData.fileName = "";
    m_metaData.date = QDateTime(QDate(1900,1,1));
    m_z = new QList<double>;
    m_qc = new QList<double>;
    m_pw = new QList<double>;
    m_wg = new QList<double>;
}

CPT::~CPT()
{
    m_z->clear();
    m_qc->clear();
    m_pw->clear();
    m_wg->clear();
}

/*
    Return a list of strings containing the values like;
    -1.23;1.234;0.013;0.562

    where these values stand for

    z [m tov ref];qc [MPa];fs [MPa];wg [%]
*/
QByteArray CPT::dataAsQByteArray()
{
    QByteArray result;
    for(int i=0; i<m_z->count();i++){
        result.append(QString("%1;%2;%3;%4\n")\
                      .arg(m_z->at(i), 2, 'f')\
                      .arg(m_qc->at(i), 3, 'f')\
                      .arg(m_pw->at(i), 3, 'f')\
                      .arg(m_wg->at(i), 3, 'f'));
    }
    return result;
}

/*
    Returns the values at the given depth. Returns NULL if z is out of range
*/
double CPT::getQcAt(double z){
    for(int i=0; i<m_qc->count(); i++)
        if (m_z->at(i) < z)
            return m_qc->at(i);
    //TODO: raise exception
    return 0.;
}

double CPT::getWgAt(double z)
{
    for(int i=0; i<m_wg->count(); i++)
        if (m_z->at(i) < z)
            return m_wg->at(i);
    //TODO: raise exception
    return 0.;
}

double CPT::getPwAt(double z)
{
    for(int i=0; i<m_pw->count(); i++)
        if (m_z->at(i) < z)
            return m_pw->at(i);
    //TODO: raise exception
    return 0.;
}

/*
    Reads a CPT from a GEF file.
*/
bool CPT::readFromFile(const QString filename, QStringList &log)
{
    qDebug() << QString("CPT::readFromFile(%1)").arg(filename);
    bool readHeader = true;
    bool hasXY = false;
    bool ok; //used to check all string -> float / int conversions
    int colid[4] = {-1, -1, -1, -1}; //dz, qc, pw, wg
    int colvoid[4] = {9999, 9999, 9999, 9999}; ////dz, qc, pw, wg

    QString typegef = "notset";
    QChar columnseperator = ' ';

    QFile file(filename);    
    //try to open the file
    if(!file.open(QIODevice::ReadOnly)) {        
        log.append(QString("ERROR in file %1: %2").arg(filename).arg(file.errorString()));
        return false;
    }

    //extract the filename
    QFileInfo fi(filename);
    m_metaData.name = fi.fileName().split('.')[0];
    m_metaData.fileName = file.fileName();

    //and read the file
    QTextStream in(&file);

    while(!in.atEnd()) {
        QString line = in.readLine();
        if(readHeader){
            QString keyword = line.split('=')[0].trimmed();
            QStringList args = line.split('=')[1].split(',');
            if (line.contains("#EOH")){
                //als er geen qc en pw is zijn we niet geinteresseerd in de sondering
                if ((colid[1]==-1) || (colid[2]==-1)){
                    log.append(QString("ERROR in file %1: Found gef file without qc or fs.").arg(filename));
                    return false;
                }
                if (colid[0]==-1){
                    log.append(QString("ERROR in file %1: Found gef file without columninfo for z.").arg(filename));
                    return false;
                }
                //stop met de header en start het lezen van de data
                readHeader = false;
            }else if (keyword =="#STARTDATE"){
                int year = args.at(0).trimmed().toInt();
                int month = args.at(1).trimmed().toInt();
                int day = args.at(2).trimmed().toInt();
                m_metaData.date = QDateTime(QDate(year, month, day));

            }else if (keyword =="#FILEDATE"){ //some people skip the startdate which is stupid but the filedate will do in this case
                if (m_metaData.date.date().year() == 1900){
                    int year = args.at(0).trimmed().toInt();
                    int month = args.at(1).trimmed().toInt();
                    int day = args.at(2).trimmed().toInt();
                    m_metaData.date = QDateTime(QDate(year, month, day));
                }
            }else if (keyword =="#COLUMNSEPARATOR"){
                QString cs = args.at(0).trimmed();
                if (cs.length()>0)
                    columnseperator = cs[0]; //TODO: check, gaat dat goed.. soms is [0] de lengte van de string
            }else if ((keyword =="#REPORTCODE")||(keyword=="#PROCEDURECODE")){
                if (line.toUpper().contains("CPT-REPORT")){
                    typegef = "sondering";
                }
            }else if (keyword =="#ZID"){
                m_metaData.zmax = args.at(1).trimmed().toDouble(&ok);
                if (!ok){
                    log.append(QString("ERROR in file %1: Invalid X coord: %2").arg(filename).arg(args.at(1)));
                    return false;
                }
            }else if (keyword =="#XYID"){                
                m_metaData.x = args.at(1).trimmed().toDouble(&ok);
                if (!ok){
                    log.append(QString("ERROR in file %1: Invalid X coord: %2").arg(filename).arg(args.at(1)));
                    return false;
                }
                m_metaData.y = args.at(2).trimmed().toDouble(&ok);
                if (!ok) {
                    log.append(QString("ERROR in file %1: Invalid Y coord: %2").arg(filename).arg(args.at(1)));
                    return false;
                }
                //calculate the latitude and longitude from the rdcoords
                LatLon ll;
                ll.fromRDCoords(m_metaData.x, m_metaData.y);
                m_metaData.latitude = ll.getLatitude();
                m_metaData.longitude = ll.getLongitude();
                hasXY = true;
            }else if (keyword == "#COLUMNVOID"){
                /*
                    ga er van uit dat altijd eerst columninfo wordt ingevuld en daarna columnvoid
                    zo niet dan is er programmeerwerk nodig!
                */
                if (colid[1]==-1){
                    log.append(QString("ERROR in file %1: Found gef file with columnvoid defined before columninfo").arg(filename));
                    return false;
                }
                int id = args.at(0).trimmed().toInt();
                for(int i=0; i<4; i++){
                    if(colid[i] == id){
                        bool ok;
                        colvoid[i] = static_cast<int>(args.at(1).trimmed().toDouble(&ok));
                        //TODO: wat als het geen int is.. hypothetisch misschien maar toch
                    }
                }
            }else if (keyword == "#COLUMNINFO"){
                int id = args.at(3).trimmed().toInt();
                if((id==1)||(id==11)){ //sondeerlengte of gecorrigeerde sondeerlengte
                    colid[0] = args.at(0).trimmed().toInt() - 1;
                }else if(id == 2){ //conusweerstand
                    colid[1] = args.at(0).trimmed().toInt() - 1;
                }else if(id == 3){ //wrijvingsweerstand
                    colid[2] = args.at(0).trimmed().toInt() - 1;
                }else if(id == 4){ //wrijvingsgetal
                    colid[3] = args.at(0).trimmed().toInt() - 1;
                }
            }
        }else{ //read data
            if (typegef!="sondering"){
                log.append(QString("ERROR in file %1: Not of type CPT").arg(filename));
                return false;
            }
            else if(!hasXY){
                log.append(QString("ERROR in file %1: No coordinates found (#XYID)").arg(filename));
                return false;
            }
            else{
                QStringList args; //filtered arguments
                QStringList _args = line.split(columnseperator); //non filtered arguments
                for (int i=0; i<_args.count(); i++){
                    if (_args.at(i).length()>0){ //if the argument is larger than 1 character add it to the filtered arguments
                        args.append(_args.at(i).trimmed());
                    }
                }

                double qc = args.at(colid[1]).toDouble(&ok);
                if (!ok){
                    log.append(QString("ERROR in file %1: Invalid qc value: %2").arg(filename).arg(args.at(colid[1])));
                    return false;
                }
                double pw = args.at(colid[2]).toDouble(&ok);
                if (!ok){
                    log.append(QString("ERROR in file %1: Invalid qc value: %2").arg(filename).arg(args.at(colid[1])));
                    return false;
                }
                if((int(qc)!=colvoid[1]) && (int(pw)!=colvoid[2])){
                    double dz = args.at(colid[0]).toDouble(&ok);
                    if (!ok){
                        log.append(QString("ERROR in file %1: Invalid dz value: %2").arg(filename).arg(args.at(colid[1])));
                        return false;
                    }

                    if(qc <= 0.)
                        qc = 0.01;
                    double pw = args.at(colid[2]).toDouble();
                    double wg;
                    if(colid[3]==-1){
                        wg = (pw / qc) * 100.;
                    }else{
                        wg = args.at(colid[3]).toDouble();
                    }
                    m_z->append(m_metaData.zmax - std::abs(dz));
                    m_qc->append(qc);
                    m_pw->append(pw);
                    m_wg->append(wg);
                }
            }
        }
    }
    m_metaData.zmin = m_z->at(m_z->count()-1);
    file.close(); //TODO: in try except manier?
    return true;
}

/*
    Returns a soiltype according to the table in CUR162 (electrical cone)
    TODO: replace hard coded ids to database ids based on the soil name
 */
int getSoiltypeByWgAndCUR162(double wg){
    if (wg <= 0.6)
        return 10000;
    else if ((wg > 0.6) && (wg <= 0.8))
        return 10001;
    else if ((wg > 0.8) && (wg <= 1.1))
        return 10002;
    else if ((wg > 1.1) && (wg <= 1.4))
        return 10003;
    else if ((wg > 1.4) && (wg <= 1.8))
        return 10004;
    else if ((wg > 1.8) && (wg <= 2.2))
        return 10005;
    else if ((wg > 2.2) && (wg <= 2.5))
        return 10006;
    else if ((wg > 2.5) && (wg <= 5.0))
        return 10007;
    else if ((wg > 5.0) && (wg <= 8.1))
        return 10008;
    else
        return 10009;
}

/*
    Walks trough a cpt and makes soillayers every interval based on the
    wg TODO: translate wg to something English
 */
void CPT::generateVSoil(VSoil &vsoil, double minInterval)
{
    vsoil.setLatitude(m_metaData.latitude);
    vsoil.setLongitude(m_metaData.longitude);
    vsoil.setX(m_metaData.x);
    vsoil.setY(m_metaData.y);
    vsoil.setSource("CPT conversion");

    double ztop = zmax();
    int n = 0;
    double sum_wg = 0.;
    for(int i=0; i<m_z->count(); i++){
        double z = m_z->at(i);
        double wg = m_wg->at(i);
        sum_wg += wg;
        n += 1;

        if(i==m_z->count()-1){
            vsoil.addSoilLayer(ztop, z, getSoiltypeByWgAndCUR162(sum_wg / n));
        }else if((ztop - z) > minInterval){
            if(n==0){
                //TODO ERROR
            }else{
                vsoil.addSoilLayer(ztop, z, getSoiltypeByWgAndCUR162(sum_wg / n));
                ztop = z;
                sum_wg = 0.;
                n = 0;
            }
        }    
    }
    vsoil.optimize();
}










