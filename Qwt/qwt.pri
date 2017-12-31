INCLUDEPATH  += $$PWD/include

DEFINES    += QT_DLL QWT_DLL

qwtLib.path = $$DESTDIR

mingw: {
    LIBS += -L$$PWD/lib_mingw/*.dll
    qwtLib.files = $$PWD/lib_mingw/
}

win32-msvc2013: {
    equals(QT_VERSION, 5.4.2) {
        LIBS += -L$$PWD/lib_vs2013/
        qwtLib.files = $$PWD/lib_vs2013_542/*.dll
        message("use qt5.4.2")
    }

    equals(QT_VERSION, 5.5.1) {
        LIBS += -L$$PWD/lib_vs2013/
        qwtLib.files = $$PWD/lib_vs2013/*.dll
        message("use qt5.5.1")
    }

    equals(QT_VERSION, 5.6.0) {
        LIBS += -L$$PWD/lib_vs2013_560/
        qwtLib.files = $$PWD/lib_vs2013_560/*.dll
        message("use qt5.6.0")
    }
    message("config vs2013, using msvc2013 lib")
}

win32-msvc2012: {
    LIBS += -L$$PWD/lib_vs2012/
    qwtLib.files = $$PWD/lib_vs2012/*.dll
    message("config vs2012, using msvc2012 lib")
}

win32-msvc2015: {
    equals(QT_VERSION, 5.7.0) {
        LIBS += -L$$PWD/lib_vs2015_570/
        qwtLib.files = $$PWD/lib_vs2015_570/*.dll
        message("use vs2015 qt5.7.0")
    }
}

CONFIG(debug, debug|release) {
    qwtLib.files += $$[QT_INSTALL_BINS]/Qt5PrintSupportd.dll
    qwtLib.files += $$[QT_INSTALL_BINS]/Qt5OpenGLd.dll
}

CONFIG(release, debug|release) {
    qwtLib.files += $$[QT_INSTALL_BINS]/Qt5PrintSupport.dll
    qwtLib.files += $$[QT_INSTALL_BINS]/Qt5OpenGL.dll
}

INSTALLS += qwtLib

CONFIG(debug, debug|release):LIBS +=  -lqwtd
CONFIG(release, debug|release):LIBS += -lqwt

