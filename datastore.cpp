#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QProgressDialog>
#include <QXmlStreamWriter>

#include "datastore.h"
#include "cpt.h"
#include "latlon.h"
#include "cmath"

DataStore::DataStore(QObject *parent) :
    QObject(parent)
{
    //init a database
    m_db = new DBAdapter(NULL);
    m_dataLoaded = FALSE;
}

DataStore::~DataStore()
{
    //close the database and cleanup
    m_db->closeDB();
    delete m_db;
    //delete generated profiles
    for(int i=0; i<m_geoProfile2Ds.count(); i++){
        delete m_geoProfile2Ds[i];
    }
    m_geoProfile2Ds.clear();
}

bool DataStore::loadDataNonUI(QString fileName)
{
    m_dataLoaded = m_db->openDB(fileName);
    m_fileName = fileName;
    m_db->getAllCPTs(m_cptsMetaData);
    m_db->getAllSoilTypes(m_soilTypes);
    m_db->getAllVSoils(m_vsoils);
    return m_dataLoaded;
}

void DataStore::loadData(QString fileName)
{
    //check if the database is open
    if (m_db->isOpen()){
        QMessageBox::warning(
            NULL,
            tr("qBB3D"),
            tr("You need to close the current database first.") );
        return;
    }
    //now open the file
    if (!m_db->openDB(fileName)){
        QMessageBox::critical(
            NULL,
            tr("qBB3D"),
            tr("Could not open database file.") );
        return;
    }
    //read all information into memory
    m_fileName = fileName;

    QProgressDialog progress("Reading database file...", tr("Cancel"), 0, 4, NULL);
    progress.setCancelButton(NULL);
    progress.show();
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(1);
    m_db->getAllCPTs(m_cptsMetaData);
    progress.setValue(2);
    m_db->getAllSoilTypes(m_soilTypes);
    progress.setValue(3);
    m_db->getAllVSoils(m_vsoils);
    m_dataLoaded = TRUE;
}

QList<VSoil*> DataStore::getVisibleVSoils(QRectF boundary)
{
    QList<VSoil *> result;
    foreach (VSoil *vs, m_vsoils){
        if ((vs->latitude() >= boundary.bottom()) && (vs->latitude() <= boundary.top()) &&
            (vs->longitude() >= boundary.left()) && (vs->longitude() <= boundary.right())){
             result.append(vs);
        }
    }
    return result;
}

QList<sCPTMetaData> DataStore::getVisibleCPTs(QRectF boundary)
{
    QList<sCPTMetaData> result;
    foreach (sCPTMetaData md, m_cptsMetaData){
        if ((md.latitude >= boundary.bottom()) && (md.latitude <= boundary.top()) &&
            (md.longitude >= boundary.left()) && (md.longitude <= boundary.right())){
             result.append(md);
        }
    }
    return result;
}

//return the vsoil.id with the coordinates closest to the given point xy
int DataStore::getVSoilIdClosestTo(QPointF xy)
{
    double dlmin = 1.e9;
    int idx = -1;
    for(int i=0; i<m_vsoils.count(); i++){
        double dx = xy.x() - m_vsoils[i]->x();
        double dy = xy.y() - m_vsoils[i]->y();
        double dl = dx * dx + dy * dy;
        if(dl < dlmin){
            idx = m_vsoils[i]->id();
            dlmin = dl;
        }
    }
    return idx;
}

/*
  Import CPTs from a given path,
  !NOTE!
  The CPT is read entirely and put into the database
  After that only the metadata is saved to use in this program
  To load the data from the cpt again you need to call it explicitly!
  */
void DataStore::importCPTS(QString path, QStringList &log)
{
    log.append("LOGBOOK import CPT files");

    QDir dir = QDir(path);
    QStringList files;
    QSqlError err;
    QFileInfoList fileinfo = dir.entryInfoList(QStringList("*.gef"),
                                               QDir::Files | QDir::NoSymLinks);
    //use the filepath and add it to the file list
    for (int i=0; i<fileinfo.count(); i++){
        files.append(fileinfo[i].filePath());
    }
    emit sendTotalCPT(files.count()); //send a signal to the dialog with the number of found cpt's

    //add cpt one by one
    for(int i=0; i<files.count(); i++){
        emit importingNextCPT(i);
        CPT *cpt = new CPT();
        VSoil vs;
        vs.setName("imported"); //TODO: set to cpt name
        if(!cpt->readFromFile(files[i], log)){
            log.append(QString("SKIPPED file %1 because of previous file read error.").arg(files[i]));
        }else{
            //check if there's another entry in the database with the same xy coords
            if (m_db->isUniqueCPT(QPointF(cpt->x(), cpt->y()))){ //if so.. add it to the database
                cpt->generateVSoil(vs, 0.1); //TODO: 0.1 vast waarde?
                m_db->addCPT(cpt, vs.id(), err);
                if(err.isValid()){
                    qDebug() << "DBERROR: %1" << err;
                    log.append(QString("SKIPPED file %1 because of database error %2").arg(files[i]).arg(err.text()));
                }
                m_db->addVSoil(vs, err);
                if(err.isValid()){
                    qDebug() << "DBERROR: %1" << err;
                    log.append(QString("SKIPPED file %1 because of database error %2").arg(files[i]).arg(err.text()));
                }
            }else{
                log.append(QString("SKIPPED file %1 because the x and y coordinate are not unique.").arg(files[i]));
            }
        }        
        m_cptsMetaData.append(cpt->metaData());
        delete cpt; //be sure to erase all stuff
    }
    //TODO: not the prettiest way to do it
    //reload the vsoil
    m_vsoils.clear();
    m_db->getAllVSoils(m_vsoils);
}

bool DataStore::importVSoilFromTextFile(QString fileName, QStringList &log)
{
    QFile file(fileName);
    //try to open the file
    if(!file.open(QIODevice::ReadOnly)) {
        log.append(QString("Error opening the file %1 for import.").arg(fileName));
        return false;
    }
    //and read the file
    if(!m_dataLoaded){
        log.append("Trying to import vsoils from textfile with closed database.");
        return false;
    }

    QTextStream in(&file);
    QString line = file.readLine();
    while(!in.atEnd()) {
        if(line.contains('#')){ //header
            VSoil vs;
            vs.setSource("Geoprofile");
            vs.setName(line.remove("#"));
            //the id is automatically entered at the addVsoil function in the database class
            line = file.readLine(); //x;y
            QStringList args = line.split(';');
            double x = args[0].trimmed().toDouble();
            double y = args[1].trimmed().toDouble();
            vs.setX(x);
            vs.setY(y);
            LatLon l;
            l.fromRDCoords(x,y);
            vs.setLatitude(l.getLatitude());
            vs.setLongitude(l.getLongitude());
            line = file.readLine(); //zmax;zmin;id
            args = line.split(';');
            vs.addSoilLayer(args[0].trimmed().toDouble(),
                             args[1].trimmed().toDouble(),
                             args[2].trimmed().toInt());
            double prevz = args[1].trimmed().toDouble();
            //now read the next lines until either #, eof or whiteline
            while(!in.atEnd()){
                line = file.readLine();
                args = line.split(';');
                if (line.trimmed().length()==0) break;
                if (line.contains('#')) break;
                vs.addSoilLayer(prevz,
                                 args[0].trimmed().toDouble(),
                                 args[1].trimmed().toInt());
                prevz = args[0].trimmed().toDouble();
                if (in.atEnd()) break;
            }
            QSqlError err;
            m_db->addVSoil(vs, err);
            if(err.isValid()){
                qDebug() << "DBERROR: %1" << err;
                log.append(QString("SKIPPED file %1 because of database error %2").arg(fileName).arg(err.text()));
                return false;
            }
        }
        if(in.atEnd()) break;
    }
    file.close();
    //reload all vsoils
    m_vsoils.clear();
    m_db->getAllVSoils(m_vsoils);
    return true;
}

void DataStore::generateGeoProfile2D(QList<QPointF> &latlonPoints)
{
    GeoProfile2D *geo = new GeoProfile2D();
    geo->setZMax(-9999.); //used to store the min z value in profile
    geo->setZMin(9999.); //used to store the max z value in profile
    //initialization
    int id = -1;
    double currentLength = 0.;
    double prevLength = 0.;    

    //add the lines to the geoprofile so we always know from which line it was generated
    for(int i=0; i<latlonPoints.count(); i++)
        geo->points()->append(latlonPoints.at(i));

    //wander through all lines
    for(int i=0; i<latlonPoints.count()-1; i++){
        //get the start- and endpoint both as latlon and rdcoords
        LatLon p1 = LatLon(latlonPoints.at(i));
        QPointF p1rd = p1.asRDCoords();        
        LatLon p2 = LatLon(latlonPoints.at(i+1));
        QPointF p2rd = p2.asRDCoords();
        //how long is this line..
        int dL = int(sqrt((p2rd.x() - p1rd.x()) * (p2rd.x() - p1rd.x()) + ((p2rd.y() - p1rd.y()) * (p2rd.y() - p1rd.y()))));
        //richtingsvector.. in english?
        QPointF rv((p2rd.x() - p1rd.x()), (p2rd.y() - p1rd.y()));
        //walk across the line in 1m steps

        for(int j=0; j<=dL; j++){
            //step one meter along the line
            int x = int(p1rd.x() + j * rv.x() / dL);
            int y = int(p1rd.y() + j * rv.y() / dL);
            //find the closest vsoilid based on the current coord
            int cId = getVSoilIdClosestTo(QPointF(x,y));
            //TODO: what if cId == -1?
            if(cId > -1){ //check the limits
                VSoil *vs = getVSoilById(cId);
                if(geo->zMax() < vs->zMax())
                    geo->setZMax(vs->zMax());
                if(geo->zMin() > vs->zMin())
                    geo->setZMin(vs->zMin());
                geo->addSoilTypeIDs(vs);
            }
            if(j==0){ //first point, just set the id to the current id
                id = cId; //set the id to the newly found id
                prevLength = 0.; //set the previous length to zero
            }
            else if((j==dL)||(id!=cId)){ //last point or new id, add the line
                sArea l; //create new line
                l.start = currentLength + prevLength; //it started where the latter ended
                l.end = currentLength + j; //and now it is a the current length
                l.vsoilId = id; //with this new id
                geo->areas()->append(l); //add it to the result
                id = cId; //set the id to the new id
                prevLength = j; //and make sure the prev length equals the current length
                if(j==dL) { currentLength += j; } //keep the current length for the next line
            }
        }

    }
    geo->optimize();
    m_geoProfile2Ds.append(geo);
}

bool DataStore::exportGeoProfileToQGeoFile(const QString fileName, const int geoProfileIndex)
{
    //initialize a file
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
             QMessageBox::warning(NULL, tr("Datastore"),
                                  tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
             return false;
         }

    //write the xml
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    //get the geometry that we want to export
    GeoProfile2D *geo = m_geoProfile2Ds.at(geoProfileIndex);

    //limits
    xml.writeStartElement("Limits");
    xml.writeAttribute("left", QString("%1").arg(geo->lMin()));
    xml.writeAttribute("right", QString("%1").arg(geo->lMax()));
    xml.writeAttribute("top", QString("%1").arg(geo->zMax(), 0, 'f', 2));
    xml.writeAttribute("bottom", QString("%1").arg(geo->zMin(), 0, 'f', 2));
    xml.writeEndElement();

    //soil types
    xml.writeStartElement("SoilTypes");
    for(int i=0; i < geo->soilTypeIDs()->count(); i++){
        SoilType *st = getSoilTypeById(geo->soilTypeIDs()->at(i));
        xml.writeStartElement("soiltype");
        xml.writeAttribute("id", QString("%1").arg(st->id()));
        xml.writeAttribute("name", st->name());
        xml.writeAttribute("description", st->description());
        xml.writeAttribute("color", st->color());
        xml.writeAttribute("ydry", QString("%1").arg(st->yDry(), 0, 'f', 2));
        xml.writeAttribute("ysat", QString("%1").arg(st->ySat(), 0, 'f', 2));
        xml.writeAttribute("c", QString("%1").arg(st->c(), 0, 'f', 1));
        xml.writeAttribute("phi", QString("%1").arg(st->phi(), 0, 'f', 1));
        xml.writeAttribute("cp", QString("%1").arg(st->cp(), 0, 'f', 1));
        xml.writeAttribute("cap", QString("%1").arg(st->cap(), 0, 'f', 1));
        xml.writeAttribute("cs", QString("%1").arg(st->cs(), 0, 'f', 1));
        xml.writeAttribute("cas", QString("%1").arg(st->cas(), 0, 'f', 1));
        xml.writeAttribute("cv", QString("%1").arg(st->cv(), 0, 'f', 1));
        xml.writeAttribute("k", QString("%1").arg(st->k(), 0, 'f', 1));

        xml.writeEndElement();
    }
    xml.writeEndElement();

    //soil layers
    xml.writeStartElement("Layers");
    for(int i=0; i<geo->areas()->count(); i++){

        sArea area = geo->areas()->at(i);

        VSoil *vs = getVSoilById(area.vsoilId);

        //q3d generates rectangular soillayers so we have to translate the lines to closed polygons that represent rectangles
        for(int j=0; j<vs->getSoilLayers()->count(); j++){
            VSoilLayer topLayer = vs->getSoilLayers()->at(j);

            xml.writeStartElement("Layer");
            xml.writeAttribute("soiltype_id", QString("%1").arg(topLayer.soiltype_id));
            xml.writeStartElement("Points");

            //topleft point
            xml.writeStartElement("Point");
            xml.writeAttribute("x", QString("%1").arg(area.start));
            xml.writeAttribute("y", QString("%1").arg(topLayer.zmax, 0, 'f', 1));
            xml.writeEndElement();
            //topright point
            xml.writeStartElement("Point");
            xml.writeAttribute("x", QString("%1").arg(area.end));
            xml.writeAttribute("y", QString("%1").arg(topLayer.zmax, 0, 'f', 1));
            xml.writeEndElement();
            //bottomright point
            xml.writeStartElement("Point");
            xml.writeAttribute("x", QString("%1").arg(area.end));
            xml.writeAttribute("y", QString("%1").arg(topLayer.zmin, 0, 'f', 1));
            xml.writeEndElement();
            //bottomleft point
            xml.writeStartElement("Point");
            xml.writeAttribute("x", QString("%1").arg(area.start));
            xml.writeAttribute("y", QString("%1").arg(topLayer.zmin, 0, 'f', 1));
            xml.writeEndElement();

            xml.writeEndElement(); //Points
            xml.writeEndElement(); //Layer
        }
    }
    xml.writeEndElement(); //Layers

    //end of the document
    xml.writeEndDocument();

    //close and check for errors
    file.close();
    if(file.error()){
        QMessageBox::warning(NULL, tr("Datastore"),
                             tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false;
    }
    return true;
}

bool DataStore::exportGeoProfileSoiltypesToCSVFile(const QString fileName, const int geoProfileIndex)
{
    if(geoProfileIndex < 0 || geoProfileIndex >= m_geoProfile2Ds.count()){
        qDebug() << "Invalid geoProfileIndex called (" << geoProfileIndex << ")";
        return false;
    }

    GeoProfile2D *geo = m_geoProfile2Ds.at(geoProfileIndex);

    if(geo==NULL){
        qDebug() << "Could not find geometry with index=" << geoProfileIndex;
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Could not create file=" << fileName;
        return false;
    }

    QTextStream out(&file);
    out << "naam,ydroog,ynat,c,phi\n";

    for (int i=0; i<geo->soilTypeIDs()->count(); i++){
        SoilType *st = getSoilTypeById(geo->soilTypeIDs()->at(i));
        if(st==NULL){
            qDebug() << "Within the geometry with id=" << geoProfileIndex << " the following error occured:";
            qDebug() << "could not find soiltype by id=" << geo->soilTypeIDs()[i];
            return false;
        }
        QString line = QString("%1,%2,%3,%4,%5\n")
                .arg(st->name())
                .arg(st->yDry(), 0, 'f', 1)
                .arg(st->ySat(), 0, 'f', 1)
                .arg(st->c(), 0, 'f', 1)
                .arg(st->phi(), 0, 'f', 1);
        out << line;
    }
    file.close(); //option.. also done in destructor of QFile..
    geo = NULL;
    return true; //succes!
}

bool DataStore::exportGeoProfileToDAM(QString path, const int geoProfileIndex)
{
    //generate the soilprofiles
    if(geoProfileIndex < 0 || geoProfileIndex >= m_geoProfile2Ds.count()){
        qDebug() << "Invalid geoProfileIndex called (" << geoProfileIndex << ")";
        return false;
    }

    path.append(QDir::separator());
    GeoProfile2D *geo = m_geoProfile2Ds.at(geoProfileIndex);
    QString soilprofilescsv = path + "soilprofiles.csv";
    QString segmentscsv = path + "segments.csv";
    QString soilmaterialscsv = path + "soilmaterials.csv";
    QString locationsegmentscsv = path + "locationsegments.csv";

    if(geo==NULL){
        qDebug() << "Could not find geometry with index=" << geoProfileIndex;
        return false;
    }

    //LOCATIONSEGEMENTS.CSV
    //write the segmenten to shapefile
    QFile file(locationsegmentscsv);
    if (!file.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Could not create file=" << locationsegmentscsv;
        return false;
    }
    QTextStream out(&file);
    out << "van,tot,segment_id\n";
    //write the segment information
    for(int i=0; i<geo->areas()->count();i++){
        out << QString("%1,%2,%3\n").arg(geo->areas()->at(i).start)
               .arg(geo->areas()->at(i).end)
               .arg(geo->areas()->at(i).vsoilId);

    }
    file.close();

    //SOILPROFILES.CSV
    //now write the vsoils
    QFile fileVSoils(soilprofilescsv);
    if (!fileVSoils.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Could not create file=" << soilprofilescsv;
        return false;
    }
    QTextStream outVSoils(&fileVSoils);
    outVSoils << "soilprofile_id,top_level,soil_name\n";
    //get all unique vsoil ids (and thus avoid double entries)
    QList<int> uniqueVSoilIds;
    geo->getUniqueVSoilsIDs(uniqueVSoilIds);
    qSort(uniqueVSoilIds);
    for(int i=0; i<uniqueVSoilIds.count(); i++){
        int id = uniqueVSoilIds.at(i);
        VSoil *vs = getVSoilById(id);
        if(vs == NULL){
            qDebug() << "Could not find vsoil with id=" << id;
            return false;
        }

        QString soilProfileId = QString("profiel_%1").arg(vs->id());
        for(int j=0; j<vs->getSoilLayers()->count(); j++){
            if(vs->getSoilLayers()->count()<1){
                qDebug() << "VSoil found with no soil layers (id=" << id << ")";
                return false;
            }
            QString topLevel = QString("%1").arg(vs->getSoilLayers()->at(j).zmax, 0, 'f', 2);
            QString soilName = getSoilTypeById(vs->getSoilLayers()->at(j).soiltype_id)->name();
            outVSoils << QString("%1,%2,%3\n").arg(soilProfileId).arg(topLevel).arg(soilName);
        }
    }

    fileVSoils.close();

    //SEGMENTS.CSV
    /* Bij het deterministisch ondergrondmodel geldt dat de vsoil_id uniek is
      en overeen kan komen met het segment_id dat Deltares vraagt */
    QFile fileSegments(segmentscsv);
    if (!fileSegments.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Could not create file=" << segmentscsv;
        return false;
    }
    QTextStream outSegments(&fileSegments);
    outSegments << "segment_id,soilprofile_id,probability,calculation_type\n";
    for(int i=0; i<uniqueVSoilIds.count(); i++){
        VSoil *vs = getVSoilById(uniqueVSoilIds.at(i));
        if(vs == NULL){
            qDebug() << "Could not find vsoil with id=" << uniqueVSoilIds.at(i);
            return false;
        }
        outSegments << QString("%1,profiel_%1,100,Stability\n").arg(vs->id());
        outSegments << QString("%1,profiel_%1,100,Piping\n").arg(vs->id());
    }

    fileSegments.close();

    //SOILMATERIALS.CSV
    exportGeoProfileSoiltypesToCSVFile(soilmaterialscsv, geoProfileIndex);

    return true;
}

bool DataStore::exportGeoProfileToKMLfile(const QString fileName, const int geoProfileIndex)
{
    GeoProfile2D *geo = m_geoProfile2Ds.at(geoProfileIndex);
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
             QMessageBox::warning(NULL, tr("Datastore"),
                                  tr("Cannot write file %1:\n%2.")
                                  .arg(fileName)
                                  .arg(file.errorString()));
             return false;
         }

    QTextStream out(&file);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    out << "<Document>\n";
    out << "    <name>Todo</name>\n"; //TODO: add filename
    out << "    <Placemark>\n";
    out << "        <name>MyProfile</name>\n";
    out << "        <LineString>\n";
    out << "            <extrude>1</extrude>\n";
    out << "            <tessellate>1</tessellate>\n";
    out << "            <altitudeMode>relativeToGround</altitudeMode>\n";
    out << "            <coordinates>\n";
    for(int i=0; i<geo->points()->count();i++){
        QPointF point = geo->points()->at(i);
        QString line = QString("            %1,%2,100\n").arg(point.x(), 0, 'f', 8).arg(point.y(), 0, 'f', 8);
        out << line;
    }
    out << "            </coordinates>\n";
    out << "        </LineString>\n";
    out << "    </Placemark>\n";
    out << "</Document>\n";
    out << "</kml>\n";

    file.close(); //option.. also done in destructor of QFile..
    geo = NULL;
    return true; //succes!
    //TODO: add done message!
}

bool DataStore::exportGeoProfileToSTIfile(const QString fileName, const int geoProfileIndex, const int width)
{
    //TODO: check index with boundaries
    GeoProfile2D *geo = m_geoProfile2Ds.at(geoProfileIndex);

    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out << "Input file for D-Geo Stability : Stability of earth slopes.\n";
    out << "==============================================================================\n";
    out << "COMPANY    : Breinbaas\n";
    out << "LICENSE    : Unknown\n";
    out << "DATE       : 16-1-2013\n";
    out << "TIME       : 9:05:55\n";
    QString fn = QString("FILENAME   : %1").arg(fileName).replace("\\", "\\\\");
    out << fn << "\n";
    out << "CREATED BY : D-Geo Stability version 10.1.2.3\n";
    out << "==========================    BEGINNING OF DATA     ==========================\n";
    out << "[VERSION]\n";
    out << "Soil=1001\n";
    out << "Geometry=1000\n";
    out << "StressCurve=1000\n";
    out << "BondStressDiagram=1000\n";
    out << "D-Geo Stability=1003\n";
    out << "[END OF VERSION]\n";
    out << "\n";
    out << "[SOIL COLLECTION]\n";
    out << QString("%1 = number of items\n").arg(geo->soilTypeIDs()->count(),5);

    //Write all soil data that corresponds with the profile
    for(int i=0; i < geo->soilTypeIDs()->count(); i++){
        SoilType *st = getSoilTypeById(geo->soilTypeIDs()->at(i));
        //TODO: what if NULL?
        out << "[SOIL]\n";
        out << QString("%1\n").arg(st->name());
        //what the heck was Deltares thinking defining the colors?
        QString color = QString("0x%1%2%3").arg(st->color().mid(5,2))
                .arg(st->color().mid(3,2))
                .arg(st->color().mid(1,2));
        bool ok;
        out << QString("SoilColor=%1\n").arg(color.toUInt(&ok, 16));
        //TODO: ok check and feedback if false
        out << "SoilSoilType=2\n";
        out << "SoilUseSoilType=0\n";
        out << "SoilExcessPorePressure=0.00\n";
        out << "SoilPorePressureFactor=1.00\n";
        out << QString("SoilGamDry=%1\n").arg(st->yDry(), 0, 'f', 2);
        out << QString("SoilGamWet=%1\n").arg(st->ySat(), 0, 'f', 2);
        out << "SoilRestSlope=0\n";
        out << QString("SoilCohesion=%1\n").arg(st->c(), 0, 'f', 2);
        out << QString("SoilPhi=%1\n").arg(st->phi(), 0, 'f', 2);
        out << "SoilDilatancy=0.00\n";
        out << "SoilCuTop=0.00\n";
        out << "SoilCuBottom=0.00\n";
        out << "SoilCuGradient=0.00\n";
        out << "SoilStressTableName=\n";
        out << "SoilBondStressTableName=\n";
        out << "SoilMatStrengthType=0\n";
        out << "SoilProbInputValues=0\n";
        out << "SoilRatioCuPc=0.22\n";
        out << "SoilPc=0.00E+00\n";
        out << "StrengthIncreaseExponent=0.70\n";
        out << "SoilPOP=10.00\n";
        out << "SoilRheologicalCoefficient=0.00\n";
        out << "xCoorSoilPc=-100.000\n";
        out << "yCoorSoilPc=-100.000\n";
        out << "IsPopCalculated=0\n";
        out << "IsOCRCalculated=0\n";
        out << "SoilIsAquifer=0\n";
        out << "SoilUseProbDefaults=1\n";
        out << "SoilStdCohesion=0.00\n";
        out << "SoilStdPhi=0.00\n";
        out << "SoilStdRatioCuPc=0.00\n";
        out << "SoilStdRatioCuPcPassive=0.00\n";
        out << "SoilStdRatioCuPcActive=0.00\n";
        out << "SoilStdCu=0.00\n";
        out << "SoilStdCuTop=0.00\n";
        out << "SoilStdCuGradient=0.00\n";
        out << "SoilStdPn=0.20\n";
        out << "SoilDistCohesion=3\n";
        out << "SoilDistPhi=3\n";
        out << "SoilDistStressTable=3\n";
        out << "SoilDistRatioCuPc=3\n";
        out << "SoilDistRatioCuPcPassive=3\n";
        out << "SoilDistRatioCuPcActive=3\n";
        out << "SoilDistCu=3\n";
        out << "SoilDistCuTop=3\n";
        out << "SoilDistCuGradient=3\n";
        out << "SoilDistPn=3\n";
        out << "SoilCorrelationCPhi=0.00\n";
        out << "SoilRatioCuPcPassive=0.00\n";
        out << "SoilRatioCuPcActive=0.00\n";
        out << "SoilCuPassiveTop=0.00\n";
        out << "SoilCuPassiveBottom=0.00\n";
        out << "SoilCuActiveTop=0.00\n";
        out << "SoilCuActiveBottom=0.00\n";
        out << "SoilUniformRatioCuPc=1\n";
        out << "SoilUniformCu=1\n";
        out << "SoilDesignPartialCohesion=1.25\n";
        out << "SoilDesignStdCohesion=-1.65\n";
        out << "SoilDesignPartialPhi=1.10\n";
        out << "SoilDesignStdPhi=-1.65\n";
        out << "SoilDesignPartialStressTable=1.15\n";
        out << "SoilDesignStdStressTable=-1.65\n";
        out << "SoilDesignPartialRatioCuPc=1.15\n";
        out << "SoilDesignStdRatioCuPc=-1.65\n";
        out << "SoilDesignPartialCu=1.15\n";
        out << "SoilDesignStdCu=-1.65\n";
        out << "SoilDesignPartialPOP=1.10\n";
        out << "SoilDesignStdPOP=-1.65\n";
        out << "SoilDesignPartialRRatio=1.00\n";
        out << "SoilDesignStdRRatio=0.00\n";
        out << "SoilSoilGroup=0\n";
        out << "SoilStdPOP=0.00\n";
        out << "SoilDistPOP=2\n";
        out << "SoilHorFluctScaleCoh=50.00\n";
        out << "SoilVertFluctScaleCoh=0.25\n";
        out << "SoilNumberOfTestsCoh=1\n";
        out << "SoilVarianceRatioCoh=0.75\n";
        out << "SoilHorFluctScalePhi=50.00\n";
        out << "SoilVertFluctScalePhi=0.25\n";
        out << "SoilNumberOfTestsPhi=1\n";
        out << "SoilVarianceRatioPhi=0.75\n";
        out << "SoilRRatio=1.0000000\n";
        out << "SoilDistCu=3\n";
        out << "SoilDistCuTop=3\n";
        out << "SoilDistCuGradient=3\n";
        out << "[END OF SOIL]\n";
    }
    out << "[END OF SOIL COLLECTION]\n";
    out << "\n";
    out << "[GEOMETRY DATA]\n";
    out << "[ACCURACY]\n";
    out << "        0.0010\n";
    out << "[END OF ACCURACY]\n";
    out << "\n";
    out << "[POINTS]\n";

    VSoil *vs = getVSoilById(geo->areas()->at(0).vsoilId);
    //TODO what if NULL
    int numPoints = (vs->getSoilLayers()->count() + 1) * 2;
    out << QString("%1  - Number of geometry points -\n").arg(numPoints, 7);
    int id = 1;
    for(int i=0; i<vs->getSoilLayers()->count(); i++){
        out << QString("%1%2%3%4\n").arg(id++, 8)
               .arg(0.0, 15, 'f', 3)
               .arg(vs->getSoilLayers()->at(i).zmax, 15, 'f', 3)
               .arg(0.0, 15, 'f', 3);
        out << QString("%1%2%3%4\n").arg(id++, 8)
               .arg(width, 15, 'f', 3)
               .arg(vs->getSoilLayers()->at(i).zmax, 15, 'f', 3)
               .arg(0.0, 15, 'f', 3);
        if(i==vs->getSoilLayers()->count()-1){
            out << QString("%1%2%3%4\n").arg(id++, 8)
                   .arg(0.0, 15, 'f', 3)
                   .arg(vs->getSoilLayers()->at(i).zmin, 15, 'f', 3)
                   .arg(0.0, 15, 'f', 3);
            out << QString("%1%2%3%4\n").arg(id++, 8)
                   .arg(width, 15, 'f', 3)
                   .arg(vs->getSoilLayers()->at(i).zmin, 15, 'f', 3)
                   .arg(0.0, 15, 'f', 3);
        }
    }
    out << "[END OF POINTS]\n";
    out << "\n";
    out << "[CURVES]\n";
    out << QString("%1 - Number of curves -\n").arg(vs->getSoilLayers()->count() + 1, 4);
    for(int i=0; i<=vs->getSoilLayers()->count(); i++){
        out << QString("%1 - Curve number\n").arg(i+1, 6);
        out << "       2 - number of points on curve,  next line(s) are pointnumbers\n";
        out << QString("%1%2\n").arg(i*2+1, 10).arg(i*2+2, 6);
    }
    out << "[END OF CURVES]\n";
    out << "\n";
    out << "[BOUNDARIES]\n"; //THERES AN ERROR IN THE FILE AS BOUNDARIES START FROM ID 0!
    out << QString("%1 - Number of boundaries -\n").arg(vs->getSoilLayers()->count() + 1, 4);
    for(int i=0; i<=vs->getSoilLayers()->count(); i++){
        out << QString("%1 - Boundary number\n").arg(i, 6);
        out << "       1 - number of curves on boundary, next line(s) are curvenumbers\n";
        out << QString("%1\n").arg(vs->getSoilLayers()->count() - i + 1,10);
    }
    out << "[END OF BOUNDARIES]\n";
    out << "\n";
    out << "[USE PROBABILISTIC DEFAULTS BOUNDARIES]\n";
    out << QString("%1 - Number of boundaries -\n").arg(vs->getSoilLayers()->count()+1, 4);
    for(int i=0; i<vs->getSoilLayers()->count() + 1; i++){
        out << "  1\n";
    }
    out << "[END OF USE PROBABILISTIC DEFAULTS BOUNDARIES]\n";
    out << "\n";
    out << "[STDV BOUNDARIES]\n";
    out << QString("%1 - Number of boundaries -\n").arg(vs->getSoilLayers()->count() + 1 , 4);
    for(int i=0; i<vs->getSoilLayers()->count() + 1; i++){
        out << "   0.00000000000000E+0000\n";
    }
    out << "[END OF STDV BOUNDARIES]\n";
    out << "\n";
    out << "[DISTRIBUTION BOUNDARIES]\n";
    out << QString("%1 - Number of boundaries -\n").arg(vs->getSoilLayers()->count() + 1, 4);
    for(int i=0; i<vs->getSoilLayers()->count() + 1; i++){
        out << "  0\n";
    }
    out << "[END OF DISTRIBUTION BOUNDARIES]\n";
    out << "\n";
    out << "[PIEZO LINES]";
    out << "   0 - Number of piezometric level lines -";
    out << "[END OF PIEZO LINES]";
    out << "\n";
    out << "[PHREATIC LINE]\n";
    out << "   0 - Number of the piezometric level line acting as phreatic line -\n";
    out << "[END OF PHREATIC LINE]\n";
    out << "\n";
    out << "[WORLD CO-ORDINATES]\n";
    out << "          0.000 - X world 1 -\n";
    out << "          0.000 - Y world 1 -\n";
    out << "          0.000 - X world 2 -\n";
    out << "          0.000 - Y world 2 -\n";
    out << "[END OF WORLD CO-ORDINATES]\n";
    out << "\n";
    out << "[LAYERS]\n";
    out << QString("%1 - Number of layers -\n").arg(vs->getSoilLayers()->count(), 4);

    //DGEo Stab starts from the bottom
    int boundaryNumber = 0;
    for(int i=vs->getSoilLayers()->count()-1; i>=0; i--){
        out << QString("%1 - Layer number, next line is material of layer\n").arg(boundaryNumber + 1, 6);
        SoilType *st = getSoilTypeById(vs->getSoilLayers()->at(i).soiltype_id);
        out << QString("       %1\n").arg(st->name());
        out << "       0 - Piezometric level line at top of layer\n";
        out << "       0 - Piezometric level line at bottom of layer\n";\
        out << QString("%1 - Boundarynumber at top of layer\n").arg(boundaryNumber, 8);
        out << QString("%1 - Boundarynumber at bottom of layer\n").arg(boundaryNumber + 1, 8);
        boundaryNumber++;
    }

    out << "[END OF LAYERS]\n";
    out << "\n";
    out << "[LAYERLOADS]\n";
    out << " - Layers which are loads -\n";
    out << "\n";
    out << "[END OF LAYERLOADS]\n";
    out << "\n";
    out << "[END OF GEOMETRY DATA]\n";
    out << "[RUN IDENTIFICATION TITLES]\n";
    out << "\n";
    out << "\n";
    out << "\n";
    out << "[MODEL]\n";
    out << "  1 : Bishop\n";
    out << "  1 : C phi\n";
    out << "  0 : Probabilistic off\n";
    out << "  1 : Mean\n";
    out << "  0 : Geotextiles off\n";
    out << "  0 : Nails off\n";
    out << "  0 : Zone plot off\n";
    out << "  0 : Local measurements\n";
    out << "[END OF MODEL]\n";
    out << "[MSEEPNET]\n";
    out << " Use potential file\n";
    out << "  0 : Do not use water net of MSeep file\n";
    out << "  0 : Do not make negative pressures 0\n";
    out << "[UNIT WEIGHT WATER]\n";
    out << "     9.81 : Unit weight water\n";
    out << "[DEGREE OF CONSOLIDATION]\n";
    out << QString("%1 Number of layers\n").arg(vs->getSoilLayers()->count(), 4);

    for(int i=0; i<vs->getSoilLayers()->count(); i++){
        int id = vs->getSoilLayers()->count()-i;
        //first print the long lines
        for(int j=1; j<i/10 + 1;j++){
            if(j==1){
                out<<QString("%1      100 100 100 100 100 100 100 100 100 100\n").arg(id, 6);
            }else{
                out<<"            100 100 100 100 100 100 100 100 100 100\n";
            }
        }
        //now finish this annoying deltares off.. :-)
        QString line = "";
        if(i<10){
            line = QString("%1     ").arg(id, 6);
        }else{
            line = "           ";
        }
        for(int j=0; j<i%10 + 1; j++)
            line += " 100";
        line += "\n";
        out << line;
    }

    out << "  0    capillary water not included\n";
    out << "[degree Temporary loads]\n";

    int lines10 = vs->getSoilLayers()->count() / 10;
    for(int i=0; i<lines10; i++){
        out << "           100 100 100 100 100 100 100\n";
    }
    QString line = "          ";
    for(int i=0; i<vs->getSoilLayers()->count()%10; i++){
        line += " 100";
    }
    out << line + "\n";

    out << "  0    capillary water not included\n";
    out << "[degree Free water(Cu)]\n";
    for(int i=0; i<lines10; i++){
        out << "           100 100 100 100 100 100 100\n";
    }
    line = "          ";
    for(int i=0; i<vs->getSoilLayers()->count()%10; i++){
        line += " 100";
    }
    out << line + "\n";

    out << "[degree earth quake]\n";
    for(int i=0; i<lines10; i++){
        out << "           100 100 100 100 100 100 100\n";
    }
    line = "          ";
    for(int i=0; i<vs->getSoilLayers()->count()%10; i++){
        line += " 100";
    }
    out << line + "\n";

    out << "[CIRCLES]\n";
    out << "       15.000           25.000      11    X-direction\n";
    out << "        5.000           15.000      11    Y-direction\n";
    out << "       -5.000          -10.000      11    Tangent lines\n";
    out << "        0.000            0.000       0    no fixed point used\n";
    out << "[SPENCER SLIP DATA]\n";
    out << "            0    Number of points\n";
    out << "[SPENCER SLIP DATA 2]\n";
    out << "            0    Number of points\n";
    out << "[SPENCER SLIP INTERVAL]\n";
    out << "  2 : Slip spencer interval\n";
    out << "[LINE LOADS]\n";
    out << "  0    =  number of items\n";
    out << "[UNIFORM LOADS ]\n";
    out << "  0     = number of items\n";
    out << "[TREE ON SLOPE]\n";
    out << "0.00 = WindForce\n";
    out << "0.00 = XCoordinate\n";
    out << "0.00 = YCoordinate\n";
    out << "10.00 = width of root zone\n";
    out << "0.0 = AngleOfDistribution\n";
    out << "[END OF TREE ON SLOPE]\n";
    out << "[EARTH QUAKE]\n";
    out << "     0.000 = horizontal acceleration\n";
    out << "     0.000 = vertical acceleration\n";
    out << "     0.000 = free water moment factor\n";
    out << "[SIGMA-TAU CURVES]\n";
    out << "    0 = number of items\n";
    out << "[END OF SIGMA-TAU CURVES]\n";
    out << "[BOND STRESS DIAGRAMS]\n";
    out << "    0 = number of items\n";
    out << "[END OF BOND STRESS DIAGRAMS]\n";
    out << "[MINIMAL REQUIRED CIRCLE DEPTH]\n";
    out << "      0.00     [m]\n";
    out << "[Slip Circle Selection]\n";
    out << "IsMinXEntryUsed=0\n";
    out << "IsMaxXEntryUsed=0\n";
    out << "XEntryMin=0.00\n";
    out << "XEntryMax=0.00\n";
    out << "[End of Slip Circle Selection]\n";
    out << "[START VALUE SAFETY FACTOR]\n";
    out << "     1.000     [-]\n";
    out << "[REFERENCE LEVEL CU]\n";
    out << "           7\n";
    out << "[LIFT SLIP DATA]\n";
    out << "        0.000            0.000       1    X-direction Left\n";
    out << "        0.000            0.000       1    Y-direction Left\n";
    out << "        0.000            0.000       1    X-direction Right\n";
    out << "        0.000            0.000       1    Y-direction Right\n";
    out << "        0.000            0.000       1    Y-direction tangent lines\n";
    out << "            0                             Automatic grid calculation (1)\n";
    out << "[EXTERNAL WATER LEVELS]\n";
    out << "     0      = No water data used\n";
    out << "  0.00      = Design level\n";
    out << "  0.30      = Decimate height\n";
    out << "    1     norm = 1/10000\n";
    out << "    1 = number of items\n";
    out << "Water data (1)\n";
    out << "     1 = Phreatic line\n";
    out << "  0.00 = Level\n";
    out << " Piezo lines\n";
    out << QString("%1 - Number of layers\n").arg(vs->getSoilLayers()->count(), 4);
    for(int i=0; i<vs->getSoilLayers()->count(); i++){
        out << "       0         0 = Pl-top and pl-bottom\n";
    }
    out << "[MODEL FACTOR]\n";
    out << "            1.00 = Limit value stability factor\n";
    out << "            0.08 = Standard deviation for limit value stability factor\n";
    out << "            0.00 = Reference standard deviation for degree of consolidation\n";
    out << "          100.00 = Length of the section\n";
    out << "    0 = Use contribution of end section\n";
    out << "            0.00 = Lateral stress ratio\n";
    out << "            0.25 = Coefficient of variation contribution edge of section\n";
    out << "[CALCULATION OPTIONS]\n";
    out << "MoveCalculationGrid=1\n";
    out << "ProbCalculationType=2\n";
    out << "SearchMethod=0\n";
    out << "[END OF CALCULATION OPTIONS]\n";
    out << "[PROBABILISTIC DEFAULTS]\n";
    out << "CohesionVariationTotal=0.25\n";
    out << "CohesionDesignPartial=1.25\n";
    out << "CohesionDesignStdDev=-1.65\n";
    out << "CohesionDistribution=3\n";
    out << "PhiVariationTotal=0.15\n";
    out << "PhiDesignPartial=1.10\n";
    out << "PhiDesignStdDev=-1.65\n";
    out << " PhiDistribution=3\n";
    out << "StressTableVariationTotal=0.20\n";
    out << "StressTableDesignPartial=1.15\n";
    out << "StressTableDesignStdDev=-1.65\n";
    out << "StressTableDistribution=3\n";
    out << "RatioCuPcVariationTotal=0.25\n";
    out << "RatioCuPcDesignPartial=1.15\n";
    out << "RatioCuPcDesignStdDev=-1.65\n";
    out << "RatioCuPcDistribution=3\n";
    out << "CuVariationTotal=0.25\n";
    out << "CuDesignPartial=1.15\n";
    out << "CuDesignStdDev=-1.65\n";
    out << "CuDistribution=3\n";
    out << "POPVariationTotal=0.10\n";
    out << "POPDesignPartial=1.10\n";
    out << "POPDesignStdDev=-1.65\n";
    out << "POPDistribution=3\n";
    out << "CompressionRatioVariationTotal=0.25\n";
    out << "CompressionRatioDesignPartial=1.00\n";
    out << "CompressionRatioDesignStdDev=0.00\n";
    out << "CompressionRatioDistribution=3\n";
    out << "ConsolidationCoefTotalStdDev=20.00\n";
    out << "ConsolidationCoefDesignPartial=1.00\n";
    out << "ConsolidationCoefDesignStdDev=1.65\n";
    out << "ConsolidationCoefDistribution=2\n";
    out << "HydraulicPressureTotalStdDev=0.50\n";
    out << "HydraulicPressureDesignPartial=1.00\n";
    out << "HydraulicPressureDesignStdDev=1.65\n";
    out << "HydraulicPressureDistribution=3\n";
    out << "LimitValueBishopMean=1.00\n";
    out << "LimitValueBishopStdDev=0.08\n";
    out << "LimitValueBishopDistribution=3\n";
    out << "LimitValueVanMean=0.95\n";
    out << "LimitValueVanStdDev=0.08\n";
    out << "LimitValueVanDistribution=3\n";
    out << "[END OF PROBABILISTIC DEFAULTS]\n";
    out << "[NEWZONE PLOT DATA]\n";
    out << "        0.00 = Diketable Height [m]\n";
    out << "        0.00 = X co-ordinate indicating start of zone [m]\n";
    out << "        0.00 = Boundary of M.H.W influence at X [m]\n";
    out << "        0.00 = Boundary of M.H.W influence at Y [m]\n";
    out << "        1.19 = Required safety in zone 1a\n";
    out << "        1.11 = Required safety in zone 1b\n";
    out << "        1.00 = Required safety in zone 2a\n";
    out << "        1.00 = Required safety in zone 2b\n";
    out << "        0.00 = Left side minimum road [m]\n";
    out << "        0.00 = Right side minimum road [m]\n";
    out << "        0.90 = Required safety in zone 3a\n";
    out << "        0.90 = Required safety in zone 3b\n";
    out << "   1    Stability calculation at right side\n";
    out << "        0.50 = Remolding reduction factor\n";
    out << "        0.80 = Schematization reduction factor\n";
    out << "   1    Overtopping condition less or equal 0.1 l/m/s\n";
    out << "[HORIZONTAL BALANCE]\n";
    out << "HorizontalBalanceXLeft=0.000\n";
    out << "HorizontalBalanceXRight=0.000\n";
    out << "HorizontalBalanceYTop=0.00\n";
    out << "HorizontalBalanceYBottom=0.00\n";
    out << "HorizontalBalanceNYInterval=1\n";
    out << "[END OF HORIZONTAL BALANCE]\n";
    out << "[REQUESTED CIRCLE SLICES]\n";
    out << " 30     = number of slices\n";
    out << "[REQUESTED LIFT SLICES]\n";
    out << " 50     = number of slices\n";
    out << "[REQUESTED SPENCER SLICES]\n";
    out << " 50     = number of slices\n";
    out << "[SOIL RESISTANCE]\n";
    out << "SoilResistanceDowelAction=1\n";
    out << "SoilResistancePullOut=1\n";
    out << "[END OF SOIL RESISTANCE]\n";
    out << "[GENETIC ALGORITHM OPTIONS BISHOP]\n";
    out << "PopulationCount=30\n";
    out << "GenerationCount=30\n";
    out << "EliteCount=2\n";
    out << "MutationRate=0.200\n";
    out << "CrossOverScatterFraction=1.000\n";
    out << "CrossOverSinglePointFraction=0.000\n";
    out << "CrossOverDoublePointFraction=0.000\n";
    out << "MutationJumpFraction=1.000\n";
    out << "MutationCreepFraction=0.000\n";
    out << "MutationInverseFraction=0.000\n";
    out << "MutationCreepReduction=0.050\n";
    out << "[END OF GENETIC ALGORITHM OPTIONS BISHOP]\n";
    out << "[GENETIC ALGORITHM OPTIONS LIFTVAN]\n";
    out << "PopulationCount=40\n";
    out << "GenerationCount=40\n";
    out << "EliteCount=2\n";
    out << "MutationRate=0.250\n";
    out << "CrossOverScatterFraction=1.000\n";
    out << "CrossOverSinglePointFraction=0.000\n";
    out << "CrossOverDoublePointFraction=0.000\n";
    out << "MutationJumpFraction=1.000\n";
    out << "MutationCreepFraction=0.000\n";
    out << "MutationInverseFraction=0.000\n";
    out << "MutationCreepReduction=0.050\n";
    out << "[END OF GENETIC ALGORITHM OPTIONS LIFTVAN]\n";
    out << "[GENETIC ALGORITHM OPTIONS SPENCER]\n";
    out << "PopulationCount=50\n";
    out << "GenerationCount=50\n";
    out << "EliteCount=2\n";
    out << "MutationRate=0.300\n";
    out << "CrossOverScatterFraction=0.000\n";
    out << "CrossOverSinglePointFraction=0.700\n";
    out << "CrossOverDoublePointFraction=0.300\n";
    out << "MutationJumpFraction=0.000\n";
    out << " MutationCreepFraction=0.900\n";
    out << "MutationInverseFraction=0.100\n";
    out << "MutationCreepReduction=0.050\n";
    out << "[END OF GENETIC ALGORITHM OPTIONS SPENCER]\n";
    out << "[NAIL TYPE DEFAULTS]\n";
    out << "NailTypeLengthNail=0.00\n";
    out << "NailTypeDiameterNail=0.00\n";
    out << "NailTypeDiameterGrout=0.00\n";
    out << "NailTypeYieldForceNail=0.00\n";
    out << "NailTypePlasticMomentNail=0.00\n";
    out << "NailTypeBendingStiffnessNail=0.00E+00\n";
    out << "NailTypeUseFacingOrBearingPlate=0\n";
    out << "[END OF NAIL TYPE DEFAULTS]\n";
    out << "[END OF INPUT FILE]\n";

    file.close(); //option.. also done in destructor of QFile..
    geo = NULL;
    return true; //succes!

    //TODO: add done message!
}

VSoil *DataStore::getVSoilById(int id)
{
    for(int i=0; i<m_vsoils.count(); i++){
        if(m_vsoils.at(i)->id()==id)
            return m_vsoils.at(i);
    }
    return NULL; //TODO: Check boundaties and give meaningfull error if it's out of bounds
}

SoilType *DataStore::getSoilTypeById(int id)
{
    for(int i=0; i<m_soilTypes.count(); i++){
        if(m_soilTypes.at(i)->id()==id)
            return m_soilTypes.at(i);
    }
    return NULL; //TODO: Check boundaties and give meaningfull error if it's out of bounds
}

//returns the next possible vsoilid
int DataStore::getNextVSoilId()
{
    //create a list with all ids
    QList<int> m_ids;
    for(int i=0; i<m_vsoils.count(); i++)
        m_ids.append(m_vsoils[i]->id());

    //start from 1 and return as soon as a new unused id is found
    int id = 1;
    while(1){
        if(!m_ids.contains(id))
            return id;
        id++;
    }

}

bool DataStore::addNewVSoil(QPointF pointLatLon, QString source)
{
    VSoil *vs = new VSoil;
    vs->setId(getNextVSoilId());
    vs->setSource(source);
    vs->setLatitude(pointLatLon.x());
    vs->setLongitude(pointLatLon.y());
    LatLon l(pointLatLon);
    vs->setX(l.asRDCoords().x());
    vs->setY(l.asRDCoords().y());
    m_vsoils.append(vs);
    return true;
}

void DataStore::getVSoilSources(QStringList &sources)
{
    m_db->getVSoilSources(sources);
}

void DataStore::saveChanges()
{
    QSqlError err;
    //check and save vsoils
    for(int i=0; i<m_vsoils.count(); i++){
        if(m_vsoils[i]->dataChanged()){
            m_db->updateVSoil(m_vsoils[i], err);
            if(err.isValid()){
                qDebug() << "DBERROR: %1" << err;
            }
        }
    }
}
