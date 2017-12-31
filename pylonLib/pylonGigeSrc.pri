INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/include

LIBS += -L"$$PWD/lib/Win32"

DEFINES += GENPYLONGIGELIB_LIBRARY

HEADERS += \
    $$PWD/autoGigeCamera.h \
    $$PWD/cameraTypedef.h \
    $$PWD/gigeCamera.h

SOURCES += \
    $$PWD/autoGigeCamera.cpp \
    $$PWD/gigeCamera.cpp
