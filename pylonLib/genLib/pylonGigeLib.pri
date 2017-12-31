INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/include

LIBS += -L"$$PWD/lib"

CONFIG(debug, debug|release):LIBS += -lpylonGigeLibd
CONFIG(release, debug|release):LIBS += -lpylonGigeLib

pylonLib.files += $$PWD/bin/*.dll
CONFIG(debug, debug|release):pylonLib.files += $$PWD/lib/pylonGigeLibd.dll
CONFIG(release, debug|release):pylonLib.files += $$PWD/lib/pylonGigeLib.dll


pylonLib.path = $$DESTDIR

INSTALLS += pylonLib

HEADERS += $$PWD/include/cameraTypedef.h \
        $$PWD/include/autoGigeCamera.h \
		$$PWD/include/gigeCamera.h
