#ifndef CPTTABLEMODEL_H
#define CPTTABLEMODEL_H

#include <QAbstractTableModel>
#include "cpt.h"

class CPTTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit CPTTableModel(QObject *parent = 0);
    explicit CPTTableModel(QList<sCPTMetaData> cpts, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const; //name, date, x, y, zmax, zmin
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;



private:
    QList<sCPTMetaData> m_cpts;
    
signals:
    
public slots:
    
};

#endif // CPTTABLEMODEL_H
