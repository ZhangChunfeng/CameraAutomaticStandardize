INCLUDEPATH  += $$PWD

HEADERS  += \
    $$PWD/hkNVRDriver.h
SOURCES += \
    $$PWD/hkNVRDriver.cpp

SERVER {
    HEADERS  += \
      $$PWD/hkNVRSrvDevice.h

    SOURCES += \
        $$PWD/hkNVRSrvDevice.cpp
}

INCLUDEPATH  += $$PWD/HCNetSDK/include

LIBS += -L$$PWD/HCNetSDK/lib \
         -lHCNetSDK -lPlayCtrl

hkLib.path = $$DESTDIR
hkLib.files = $$PWD/HCNetSDK/bin/*

INSTALLS += hkLib

