INCLUDEPATH += $$PWD

LIBS += -L$$PWD

CONFIG(debug, debug|release): LIBS += -lscan5Sdkd
CONFIG(release, debug|release):LIBS += -lscan5Sdk

CONFIG(debug, debug|release):scan5Install.files += $$PWD/scan5Sdkd.dll
CONFIG(release, debug|release):scan5Install.files += $$PWD/scan5Sdk.dll
scan5Install.path = $$DESTDIR

INSTALLS += scan5Install
