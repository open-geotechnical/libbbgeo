#ifndef SOILTYPETABLEMODEL_H
#define SOILTYPETABLEMODEL_H

#include <QAbstractTableModel>

#include "soiltype.h"

class SoilTypeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SoilTypeTableModel(QObject *parent = 0);
    explicit SoilTypeTableModel(QList<SoilType*> soilTypes, QObject *parent = 0, bool maximized=true);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const; //name, date, x, y, zmax, zmin
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    QList<SoilType*> m_soilTypes;
    bool m_maximized;

signals:
    
public slots:
    
};

#endif // SOILTYPETABLEMODEL_H
