#ifndef DATASTORE_H
#define DATASTORE_H

#include <QObject>
#include <QRectF>

#include "cpt.h"
#include "dbadapter.h"
#include "geoprofile2d.h"

#include <QPointF>

class DataStore : public QObject
{
    Q_OBJECT
public:
    explicit DataStore(QObject *parent = 0);
    ~DataStore();

    void loadData(QString filename);
    bool loadDataNonUI(QString fileName);
    QList<sCPTMetaData> getVisibleCPTs(QRectF boundary);
    QList<VSoil *> getVisibleVSoils(QRectF boundary);
    int getNumberOfCPTs() { return m_cptsMetaData.count(); }
    int getNumberOfSoilTypes() { return m_soilTypes.count(); }
    int getVSoilIdClosestTo(QPointF xy);

    void importCPTS(QString path, QStringList &log);
    bool importVSoilFromTextFile(QString fileName, QStringList &log);

    void generateGeoProfile2D(QList<QPointF> &latlonPoints);

    QList<sCPTMetaData> getCPTMetaDatas() { return m_cptsMetaData; }
    QList<GeoProfile2D*> getProfiles() { return m_geoProfile2Ds; }
    QList<SoilType*> getSoilTypes() { return m_soilTypes; }
    QList<VSoil*> getVSoils() { return m_vsoils; }

    VSoil* getVSoilById(int id);
    SoilType* getSoilTypeById(int id);
    QString fileName() { return m_fileName; }


    /*
     * EXPORT OPTIONS
     */
    bool exportGeoProfileToSTIfile(const QString fileName, const int geoProfileId, const int width);
    bool exportGeoProfileToKMLfile(const QString fileName, const int geoProfileIndex);
    bool exportGeoProfileToQGeoFile(const QString fileName, const int geoProfileIndex);
    bool exportGeoProfileSoiltypesToCSVFile(const QString fileName, const int geoProfileIndex);
    bool exportGeoProfileToDAM(const QString path, const int geoProfileIndex);

    bool dataLoaded() { return m_dataLoaded; }

    int getNextVSoilId();
    bool addNewVSoil(QPointF pointLatLon, QString source);
    void getVSoilSources(QStringList &sources);
    void getVSoilLocations(QStringList &locations);


public slots:
    void saveChanges();

private:
    DBAdapter *m_db;    
    QList<sCPTMetaData> m_cptsMetaData; //a list containing all cpt's in the database
    QList<SoilType*> m_soilTypes; //a list containing all soiltypes from the db
    QList<VSoil*> m_vsoils; //a list containing all vsoil from the db
    QList<GeoProfile2D*> m_geoProfile2Ds; //a list containing all generated 2D geotechnical profiles

    QString m_fileName; //the name of the database file

    void getSoilTypesByProfile(GeoProfile2D *geo, QList<SoilType*> &soilTypes);

    bool m_dataLoaded; //returns true if data is loaded into the store

signals:
    void importingNextCPT(int currentCPTNumber);
    void sendTotalCPT(int numCPTs);


};

#endif // DATASTORE_H
