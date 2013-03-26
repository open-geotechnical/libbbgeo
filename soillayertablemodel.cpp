#include "soillayertablemodel.h"

SoilLayerTableModel::SoilLayerTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    m_soilLayers = NULL;
}

SoilLayerTableModel::SoilLayerTableModel(QList<VSoilLayer> *soilLayers, QObject *parent)
{
    Q_UNUSED(parent);
    m_soilLayers = soilLayers;
}

int SoilLayerTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_soilLayers->count();
}

int SoilLayerTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant SoilLayerTableModel::data(const QModelIndex &index, int role) const
{
    if((!index.isValid())||(index.row()<0)||(index.row()>m_soilLayers->count()))
        return QVariant();
    else{
        VSoilLayer sl = m_soilLayers->at(index.row());
        if(role == Qt::DisplayRole){
            switch(index.column()){
                case 0: return QVariant(QString("%1").arg(sl.zmax, 0, 'f', 2));
                case 1: return QVariant(QString("%1").arg(sl.zmin, 0, 'f', 2));
                case 2: return QVariant(QString("%1").arg(sl.soiltype_id));
                default: return QVariant();
            }
        }
        else if(role==Qt::DecorationRole){
            if(index.column()==1){
                VSoilLayer vs = m_soilLayers->at(index.row());
                if(vs.zmax <= vs.zmin){
                    return Qt::red;
                }
            }
            if(index.column()==0 && index.row()==0){
                VSoilLayer vs = m_soilLayers->at(index.row());
                if(vs.zmax <= vs.zmin){
                    return Qt::red;
                }
            }
            //TODO: check validity soiltype id
        }
    }
    return QVariant();
}

QVariant SoilLayerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return tr("zmax");
            case 1: return tr("zmin");
            case 2: return tr("soiltype_id");
            default: return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags SoilLayerTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if(index.row()==0)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else if(index.row()>0 && index.column()>0)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled;
}

bool SoilLayerTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        int row = index.row();
        int col = index.column();

        VSoilLayer vs = m_soilLayers->at(row);

        switch(col){
            case 0: vs.zmax = value.toDouble(); break;
            case 1: vs.zmin = value.toDouble(); break;
            case 2: vs.soiltype_id = value.toInt(); break;
            default: return false;
        }
        m_soilLayers->replace(row, vs);

        //as a convinience change the zmax of the lower soillayer to the zmin of this soillayer
        if(col==1 && row < m_soilLayers->count()-1){
            VSoilLayer vs2 = m_soilLayers->at(row+1);
            vs2.zmax = vs.zmin;
            m_soilLayers->replace(row+1, vs2);
        }
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

bool SoilLayerTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++) {
        VSoilLayer vs;
        vs.soiltype_id = 0;

        if(position==0){
            if(rowCount(index)>0){
                vs.zmin = m_soilLayers->at(position).zmax;
                vs.zmax = vs.zmin + 1.0;
            }else{ //first entry
                vs.zmin = -1.0;
                vs.zmax = 0.0;
            }
        }else{
            vs.zmax = m_soilLayers->at(position-1).zmin;
            vs.zmin = vs.zmax - 0.1;
        }
        vs.soiltype_id = -1;
        m_soilLayers->insert(position, vs);
    }

    endInsertRows();    
    return true;
}

bool SoilLayerTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        m_soilLayers->removeAt(position);
    }

    endRemoveRows();

    return true;
}

void SoilLayerTableModel::removeAllRows()
{
    removeRows(0, rowCount(QModelIndex()), QModelIndex());
}
