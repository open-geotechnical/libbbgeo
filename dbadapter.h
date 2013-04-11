#ifndef DBADAPTER_H
#define DBADAPTER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QPointF>

#include "soiltype.h"
#include "vsoil.h"
#include "cpt.h"

class DBAdapter : public QObject
{
    Q_OBJECT
public:
    explicit DBAdapter(QObject *parent = 0);
    ~DBAdapter();
    bool openDB(QString filename);
    void closeDB();
    bool isOpen();

    void getAllCPTs(QList<sCPTMetaData> &cptsMetaData);
    void getAllSoilTypes(QList<SoilType *> &soilTypes);
    void getAllVSoils(QList<VSoil *> &vsoils);

    void addCPT(CPT *cpt, const int vsoilId, QSqlError &err);
    void addVSoil(VSoil &vsoil, QSqlError &err);
    void updateVSoil(VSoil *vsoil, QSqlError &err);

    bool isUniqueCPT(QPointF point);
    bool isUniqueVSoil(QPointF point);
    void getVSoilSources(QStringList &sources);

private:
    QSqlDatabase m_db;
    int getMaxIDFromCPT();
    int getMaxIDFromVSoil();

signals:
    
public slots:
    
};

#endif // DBADAPTER_H
