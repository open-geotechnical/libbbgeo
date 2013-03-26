#include "dbadapter.h"

#include <QDebug>
#include <QSqlQuery>


DBAdapter::DBAdapter(QObject *parent) :
    QObject(parent)
{
}

DBAdapter::~DBAdapter()
{
    if (m_db.isOpen()){
        //qDebug() << "CLOSING DB";
        closeDB();
    }
}

void DBAdapter::closeDB()
{
    m_db.close();
}

/*
  Return the highest id (int) in the cpt table
 */
int DBAdapter::getMaxIDFromCPT()
{
    QSqlQuery qry;
    qry.exec("SELECT max(id) FROM cpt");
    if (qry.first())
        return qry.value(0).toInt();
    else
        return 0;
}

int DBAdapter::getMaxIDFromVSoil()
{
    QSqlQuery qry;
    qry.exec("SELECT max(id) FROM vsoil");
    if (qry.first())
        return qry.value(0).toInt();
    else
        return 0;

}

void DBAdapter::getAllCPTs(QList<sCPTMetaData> &cptsMetaData)
{
    QSqlQuery qry;
    sCPTMetaData md;

    cptsMetaData.clear();

    qry.exec("SELECT * FROM cpt");
    while (qry.next()) {
        md.id = qry.value(0).toInt();
        md.date = qry.value(1).toDateTime();
        md.x = qry.value(2).toDouble();
        md.y = qry.value(3).toDouble();
        md.zmax = qry.value(4).toDouble();
        md.zmin = qry.value(5).toDouble();
        md.fileName = qry.value(6).toString();
        md.latitude = qry.value(8).toDouble();
        md.longitude = qry.value(9).toDouble();
        md.name = qry.value(10).toString();
        cptsMetaData.append(md);
    }
    //qDebug() << "Aantal sonderingen: " << cptsMetaData.count();
}

void DBAdapter::getAllSoilTypes(QList<SoilType*> &soilTypes)
{
    soilTypes.clear();
    QSqlQuery qry;
    SoilType *st;
    qry.exec("SELECT * FROM soiltypes");
    while (qry.next()) {
        st = new SoilType();
        st->setId(qry.value(0).toInt());
        st->setName(qry.value(1).toString());
        st->setDescription(qry.value(2).toString());
        st->setSource(qry.value(3).toString());
        st->setYdry(qry.value(4).toDouble());
        st->setYsat(qry.value(5).toDouble());
        st->setC(qry.value(6).toDouble());
        st->setPhi(qry.value(7).toDouble());
        st->setUpsilon(qry.value(8).toDouble());
        st->setK(qry.value(9).toDouble());
        st->setMCUpsilon(qry.value(10).toDouble());
        st->setMCE50(qry.value(11).toDouble());
        st->setHSE50(qry.value(12).toDouble());
        st->setHSEoed(qry.value(13).toDouble());
        st->setHSEur(qry.value(14).toDouble());
        st->setHSm(qry.value(15).toDouble());
        st->setSSClambda(qry.value(16).toDouble());
        st->setSSClambda(qry.value(17).toDouble());
        st->setSSCkappa(qry.value(18).toDouble());
        st->setCp(qry.value(19).toDouble());
        st->setCs(qry.value(20).toDouble());
        st->setCap(qry.value(21).toDouble());
        st->setCas(qry.value(22).toDouble());
        st->setCv(qry.value(23).toDouble());
        st->setColor(qry.value(24).toString());
        soilTypes.append(st);
    }
    //qDebug() << "Aantal grondsoorten: " << soilTypes.count();
}

void DBAdapter::getAllVSoils(QList<VSoil *> &vsoils)
{
    vsoils.clear();
    QSqlQuery qry;
    VSoil *vs;
    qry.exec("SELECT * FROM vsoil");
    while (qry.next()) {
        vs = new VSoil();
        vs->setId(qry.value(0).toInt());
        vs->setX(qry.value(1).toDouble());
        vs->setY(qry.value(2).toDouble());
        vs->setLatitude(qry.value(3).toDouble());
        vs->setLongitude(qry.value(4).toDouble());
        vs->setSource(qry.value(5).toString());
        vs->blobToData(qry.value(6).toString());
        vs->setName(qry.value(7).toString());
        vsoils.append(vs);
    }
    //qDebug() << "Aantal vsoil: " << vsoils.count();
}

void DBAdapter::addCPT(CPT *cpt, const int vsoilId, QSqlError &err)
{
    cpt->setId(getMaxIDFromCPT() + 1);
    QByteArray blob = cpt->dataAsQByteArray();
    QSqlQuery qry;    
    qry.prepare("INSERT INTO cpt VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    qry.bindValue(0, cpt->id());
    qry.bindValue(1, cpt->date());
    qry.bindValue(2, cpt->x());
    qry.bindValue(3, cpt->y());
    qry.bindValue(4, cpt->zmax());
    qry.bindValue(5, cpt->zmin());
    qry.bindValue(6, cpt->fileName());
    qry.bindValue(7, vsoilId);
    qry.bindValue(8, cpt->latitude());
    qry.bindValue(9, cpt->longitude());
    qry.bindValue(10, cpt->name());
    qry.exec();    
    err = qry.lastError();
}

/*
  Returns true if there is no entry in the cpt table that
  has the x,y coords given in point.
 */
bool DBAdapter::isUniqueCPT(QPointF point)
{
    QSqlQuery qry;
    qry.prepare("SELECT * FROM cpt WHERE x=? AND y=?");
    qry.bindValue(0, point.x());
    qry.bindValue(1, point.y());
    qry.exec();    
    return qry.first()==QVariant::Invalid;
}

void DBAdapter::addVSoil(VSoil &vsoil, QSqlError &err)
{
    vsoil.setId(getMaxIDFromVSoil() + 1);
    QByteArray blob = vsoil.dataAsQByteArray();
    QSqlQuery qry;
    qry.prepare("INSERT INTO vsoil VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    qry.bindValue(0, vsoil.id());
    qry.bindValue(1, vsoil.x());
    qry.bindValue(2, vsoil.y());
    qry.bindValue(3, vsoil.latitude());
    qry.bindValue(4, vsoil.longitude());
    qry.bindValue(5, vsoil.source());
    qry.bindValue(6, blob.data());
    qry.bindValue(7, vsoil.name());
    qry.exec();
    err = qry.lastError();
}

void DBAdapter::updateVSoil(VSoil *vsoil, QSqlError &err){
    QByteArray blob = vsoil->dataAsQByteArray();
    QSqlQuery qry;

    qry.prepare("UPDATE vsoil SET x=:x, y=:y, latitude=:lat, longitude=:lon, source=:src, data=:data, name=:name WHERE id=:id");

    qry.bindValue(":x", vsoil->x());
    qry.bindValue(":y", vsoil->y());
    qry.bindValue(":lat", vsoil->latitude());
    qry.bindValue(":lon", vsoil->longitude());
    qry.bindValue(":src", vsoil->source());
    qry.bindValue(":data", blob.data());
    qry.bindValue(":id", vsoil->id());
    qry.bindValue(":name", vsoil->name());
    qry.exec();
    vsoil->setDataChanged(FALSE);
    err = qry.lastError();
}

bool DBAdapter::isOpen()
{
    return m_db.isOpen();
}

bool DBAdapter::openDB(QString filename)
{
    //qDebug() << "OPENING DB";
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(filename);
    return m_db.open();
}
