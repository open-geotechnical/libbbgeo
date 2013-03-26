#ifndef VSOIL_H
#define VSOIL_H

#include <QObject>
#include <QString>
#include <QStringList>

struct VSoilLayer{
    double zmin;
    double zmax;
    int soiltype_id;
};

class VSoil : public QObject
{
    Q_OBJECT
public:
    explicit VSoil(QObject *parent = 0);
    ~VSoil();
    void blobToData(QString data);
    QByteArray dataAsQByteArray();

    double zMin();
    double zMax();
    double x() { return m_x; }
    double y() { return m_y; }
    double latitude() { return m_lat; }
    double longitude() { return m_lng; }
    QString name() { return m_name; }

    void optimize();

    int id() { return m_id; }
    QString source() { return m_source; }
    QList<VSoilLayer> *getSoilLayers() { return m_soilLayers; }

    void setName(QString name) { m_name = name; }
    void setId(int id) { m_id = id; }
    void setSource(QString source) { m_source = source; }
    void setX(double x) { m_x = x; }
    void setY(double y) { m_y = y; }
    void setLatitude(double lat) { m_lat = lat; }
    void setLongitude(double lng) { m_lng = lng; }

    void addSoilLayer(double zmax, double zmin, int id);
    bool dataChanged() { return m_dataChanged; }
    void setDataChanged(bool dataHasChanged) { m_dataChanged = dataHasChanged; }

private:
    int m_id;
    double m_x;
    double m_y;
    double m_lat;
    double m_lng;
    QString m_source;
    QList<VSoilLayer> *m_soilLayers;
    QString m_name;

    bool m_dataChanged;

    
signals:
    
public slots:
    
};

#endif // VSOIL_H
