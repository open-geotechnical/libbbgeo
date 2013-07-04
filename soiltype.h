#ifndef SOILTYPE_H
#define SOILTYPE_H

#include <QObject>
#include <QString>

class SoilType : public QObject
{
    Q_OBJECT
public:
    explicit SoilType(QObject *parent = 0);

    //getters
    int id() { return m_id;}
    QString name() {return m_name;}
    QString description() {return m_description;}
    QString source() {return m_source;}
    double yDry() {return m_ydry;}
    double ySat() {return m_ysat;}
    double c() {return m_c;}
    double phi() {return m_phi;}
    double upsilon() {return m_upsilon;}
    double k() {return m_k;}
    double mcUpsilon() {return m_MC_upsilon;}
    double mcE50() {return m_MC_E50;}
    double hsE50() {return m_HS_E50;}
    double hsEoed() {return m_HS_Eoed;}
    double hsEur() {return m_HS_Eur; }
    double hsM() {return m_HS_m;}
    double sscLambda() {return m_SSC_lambda;}
    double sscKappa() {return m_SSC_kappa;}
    double sscMu() {return m_SSC_mu;}
    double cp() {return m_Cp;}
    double cap() {return m_Cap;}
    double cs() {return m_Cs;}
    double cas() {return m_Cas;}
    double cv() {return m_cv;}
    QString color() {return m_color;}
    bool dataChanged() { return m_dataChanged; }

    //setters
    void setId(int i) { m_id = i;}
    void setName(QString s) {m_name = s;}
    void setDescription(QString s) {m_description = s;}
    void setSource(QString s) {m_source = s;}
    void setYdry(double d) {m_ydry = d;}
    void setYsat(double d) {m_ysat = d;}
    void setC(double d) {m_c = d;}
    void setPhi(double d) {m_phi = d;}
    void setUpsilon(double d) {m_upsilon = d;}
    void setK(double d) {m_k = d;}
    void setMCUpsilon(double d) {m_MC_upsilon = d;}
    void setMCE50(double d) {m_MC_E50 = d;}
    void setHSE50(double d) {m_HS_E50 = d;}
    void setHSEoed(double d) {m_HS_Eoed = d;}
    void setHSEur(double d) {m_HS_Eur = d;}
    void setHSm(double d) {m_HS_m = d;}
    void setSSClambda(double d) {m_SSC_lambda = d;}
    void setSSCkappa(double d) {m_SSC_kappa = d;}
    void setSSCmu(double d) {m_SSC_mu = d;}
    void setCp(double d) {m_Cp = d;}
    void setCap(double d) {m_Cap = d;}
    void setCs(double d) {m_Cs = d;}
    void setCas(double d) {m_Cas = d;}
    void setCv(double d) {m_cv = d;}
    void setColor(QString s) {m_color = s;}
    void setDataChanged(bool dataHasChanged) { m_dataChanged = dataHasChanged; }

private:
    int m_id;             //id of the soiltype
    QString m_name;       //short name of the soiltype (sand / clay / loam / peat etc
    QString m_description;//a longer descriptive name
    QString m_source;     //source of the information
    double m_ydry;      //dry weight in kN/m3
    double m_ysat;      //saturated weight in kN/m3
    double m_c;         //cohesion kN/m
    double m_phi;       //angle of friction [degrees]
    double m_upsilon;   //#TODO opzoeken
    double m_k;         //permeability in [m/day]
    double m_MC_upsilon;//PLAXIS MC model upsilon
    double m_MC_E50;    //PLAXIS MC model E50 [MPa]
    double m_HS_E50;    //PLAXIS HS model E50 [MPa]
    double m_HS_Eoed;   //PLAXIS HS model Eoedometer [MPa]
    double m_HS_Eur;    //PLAXIS HS model Eunload-reload [MPa]
    double m_HS_m;      //PLAXIS stiffness-stress dependency
    double m_SSC_lambda;//PLAXIS SSC model lambda [-]
    double m_SSC_kappa; //PLAXIS SSC model kappa [-]
    double m_SSC_mu;    //PLAXIS SSC model mu [-]
    double m_Cp;        //Compression index Cp [-]
    double m_Cs;        //Compression index Cs [-]
    double m_Cap;       //Compression index C'p [-]
    double m_Cas;       //Compression index C's [-]
    double m_cv;        //Cv value [m2/s]
    QString m_color;    //Color in HTML code, ie. #RRGGBB
    bool m_dataChanged; //shows whether the data has changed


    
signals:
    
public slots:
    
};

#endif // SOILTYPE_H
