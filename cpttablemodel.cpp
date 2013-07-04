#include "cpttablemodel.h"

CPTTableModel::CPTTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

CPTTableModel::CPTTableModel(QList<sCPTMetaData> cpts, QObject *parent)
{
    Q_UNUSED(parent)
    m_cpts = cpts;
}

int CPTTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_cpts.count();
}

int CPTTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 8; //id, name, filename, date, x, y, zmax, zmin
}

QVariant CPTTableModel::data(const QModelIndex &index, int role) const
{
    if((!index.isValid())||(index.row()<0)||(index.row()>m_cpts.count()))
        return QVariant();
    else{
        sCPTMetaData md = m_cpts[index.row()];
        if(role == Qt::DisplayRole){
            switch(index.column()){
                case 0: return QVariant(QString("%1").arg(md.id));
                case 1: return QVariant(QString("%1").arg(md.name));
                case 2: return QVariant(QString("%1").arg(md.fileName));
                case 3: return QVariant(QString("%1").arg(md.date.toString()));
                case 4: return QVariant(QString("%1").arg(md.x));
                case 5: return QVariant(QString("%1").arg(md.y));
                case 6: return QVariant(QString("%1").arg(md.zmax));
                case 7: return QVariant(QString("%1").arg(md.zmin));
                default: return QVariant();
            }
        }
    }
    return QVariant();
}

QVariant CPTTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return tr("id");
            case 1: return tr("name");
            case 2: return tr("filename");
            case 3: return tr("date");
            case 4: return tr("x");
            case 5: return tr("y");
            case 6: return tr("z;max");
            case 7: return tr("z;min");
            default:  return QVariant();
        }
    }
    return QVariant();
}
