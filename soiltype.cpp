#include "soiltype.h"

SoilType::SoilType(QObject *parent) :
    QObject(parent)
{
    m_dataChanged = false;
}
