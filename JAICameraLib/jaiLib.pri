
SDKRoot=$$(JAISDK_ROOT)
SDKRoot=$$absolute_path("", $$SDKRoot)

message($$SDKRoot)
INCLUDEPATH += $$SDKRoot/library/CPP/include

message($$SDKRoot/library/CPP/include)

LIBS += -L$$SDKRoot/library/CPP/lib/Win32_i86
LIBS += -lJai_Factory
jaiLib.files += $$SDKRoot/bin/Win32_i86/*.dll

jaiLib.path = $$DESTDIR

INSTALLS += jaiLib
