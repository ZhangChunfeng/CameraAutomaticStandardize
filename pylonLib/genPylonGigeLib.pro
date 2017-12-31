#-------------------------------------------------
#
# Project created by QtCreator 2016-08-08T14:55:26
#
#-------------------------------------------------

QT       -= gui
CONFIG   += build_all  debug_and_release  c++11

TARGET = pylonGigeLib
CONFIG(debug, debug|release):TARGET = $${TARGET}d

TEMPLATE = lib

include ($$PWD/pylonGigeSrc.pri)

DESTDIR = $$PWD/output

inc.files += $$PWD/cameraTypedef.h
inc.files += $$PWD/autoGigeCamera.h
inc.files += $$PWD/gigeCamera.h
inc.path = $$PWD/genLib/include/

priDir.files +=  $$PWD/pylonGigeLib.pri
priDir.path = $$PWD/genLib/

#libLib.files += $$PWD/output/$${TARGET}.lib
#libLib.path += $$PWD/genLib/lib/

target.path = $$PWD/genLib/lib/

libBin.files += $$PWD/bin/*
libBin.files += $$PWD/output/*.dll
libBin.path = $$PWD/genLib/bin/

INSTALLS += inc target libBin priDir

