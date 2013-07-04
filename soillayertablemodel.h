#ifndef SOILLAYERTABLEMODEL_H
#define SOILLAYERTABLEMODEL_H

#include <QAbstractTableModel>

#include "vsoil.h"

class SoilLayerTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SoilLayerTableModel(QObject *parent = 0);
    explicit SoilLayerTableModel(QList<VSoilLayer> *soilLayers, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const; //name, date, x, y, zmax, zmin
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    bool insertRows(int position, int rows, const QModelIndex &index);
    bool removeRows(int position, int rows, const QModelIndex &index);

    void removeAllRows();

    QList<VSoilLayer> *getSoilLayers() {return m_soilLayers;}

private:
    QList<VSoilLayer> *m_soilLayers;
    
signals:
    
public slots:
    
};

#endif // SOILLAYERTABLEMODEL_H
