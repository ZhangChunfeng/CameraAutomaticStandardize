#-------------------------------------------------
#
# Project created by QtCreator 2016-07-10T08:36:11
#
#-------------------------------------------------

QT       += core gui  serialport axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StandardizeTest
TEMPLATE = app

mingw:{
    DESTDIR = $$PWD/bin_mingw
    TEMPDIR = mingw
}

msvc:{
    DESTDIR = $$PWD/bin_vs
    TEMPDIR = vs
}

include (Qwt/qwt.pri)
include (hkNVRDriver/hkNVRDriver.pri)
include (ffmTool/ffmTool.pri)
include (pylonLib/genLib/pylonGigeLib.pri)
include (JAICameraLib/JAICameraLib.pri)
include (scan5Sdk_bin/scan5Sdk.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    hklogindlg.cpp \
    baslerlogindlg.cpp \
    parameterconfigdlg.cpp \
    showtabledialog.cpp \
    spcomm.cpp \
    datarelationsdialog.cpp \
    planckparameterdialog.cpp \
    doubleccddialog.cpp\
    plot.cpp \
    scan5dialog.cpp


HEADERS  += mainwindow.h \
    hklogindlg.h \
    baslerlogindlg.h \
    parameterconfigdlg.h \
    showtabledialog.h \
    spcomm.h \
    datarelationsdialog.h \
    planckparameterdialog.h \
    doubleccddialog.h\
    polyandplanckfit.h\
    plot.h \
    scan5dialog.h

FORMS    += mainwindow.ui \
    hklogindlg.ui \
    baslerlogindlg.ui \
    parameterconfigdlg.ui \
    showtabledialog.ui \
    datarelationsdialog.ui \
    planckparameterdialog.ui \
    doubleccddialog.ui \
    scan5dialog.ui

INCLUDEPATH += $$PWD/.
DEPENDPATH  += $$PWD/.

LIBS += -L$$PWD/ -lplanckFit
LIBS += -L$$PWD/ -lpoly2Fit


RESOURCES += \
    myresource.qrc
