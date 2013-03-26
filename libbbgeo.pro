#-------------------------------------------------
#
# Project created by QtCreator 2013-02-13T10:50:04
#
#-------------------------------------------------

QT       += network sql gui

TARGET = libbbgeo
TEMPLATE = lib

DEFINES += LIBBBGEO_LIBRARY

SOURCES += libbbgeo.cpp \
    vsoil.cpp \
    soiltypetablemodel.cpp \
    soiltype.cpp \
    soillayertablemodel.cpp \
    latlon.cpp \
    geoprofile2d.cpp \
    dbadapter.cpp \
    datastore.cpp \
    cpttablemodel.cpp \
    cpt.cpp

HEADERS += libbbgeo.h\
        libbbgeo_global.h \
    vsoil.h \
    soiltypetablemodel.h \
    soiltype.h \
    soillayertablemodel.h \
    latlon.h \
    geoprofile2d.h \
    dbadapter.h \
    datastore.h \
    cpttablemodel.h \
    cpt.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE902B8B0
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = libbbgeo.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    libbbgeo.pri
