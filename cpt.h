#ifndef CPT_H
#define CPT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QStringList>

#include "vsoil.h"

struct sCPTMetaData{
    int id;
    double x;
    double y;
    double latitude;
    double longitude;
    double zmax;
    double zmin;
    QString fileName;
    QDateTime date;
    QString name;
};

class CPT : public QObject
{
    Q_OBJECT
public:
    explicit CPT(QObject *parent = 0);
    ~CPT();
    QByteArray dataAsQByteArray();

    double getQcAt(double z);
    double getWgAt(double z);
    double getPwAt(double z);

    //void blobToData(QString data);

    bool readFromFile(const QString filename, QStringList &log);
    sCPTMetaData metaData() { return m_metaData; }

    int id() { return m_metaData.id; }
    double x() { return m_metaData.x; }
    double y() { return m_metaData.y; }
    double latitude() {return m_metaData.latitude; }
    double longitude() {return m_metaData.longitude; }
    double zmax() { return m_metaData.zmax; }
    double zmin() { return m_metaData.zmin; }
    QString fileName() { return m_metaData.fileName; }
    QDateTime date() { return m_metaData.date; }
    QString name() { return m_metaData.name; }

    void setId(int id) { m_metaData.id = id; }
    void setDate(QDateTime dt) { m_metaData.date = dt; }
    void setX(double x) { m_metaData.x = x; }
    void setY(double y) { m_metaData.y = y; }
    void setFileName(QString fileName) { m_metaData.fileName = fileName; }
    void setLatitude(double lat) { m_metaData.latitude = lat; }
    void setLongitude(double lon) {m_metaData.longitude = lon; }
    void setName(QString name) {m_metaData.name = name; }

    void generateVSoil(VSoil &vsoil, double minInterval);


private:
    sCPTMetaData m_metaData;

    QList<double> m_z;  //all z points
    QList<double> m_qc; //all qc points
    QList<double> m_pw; //all pw points
    QList<double> m_wg; //all wg points

signals:
    
public slots:
    
};

#endif // CPT_H
