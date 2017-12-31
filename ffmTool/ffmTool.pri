INCLUDEPATH  +=$$PWD

mingw: {
    INCLUDEPATH  +=$$PWD/ffmpeg/mingw/include
    LIBS += -L$$PWD/ffmpeg/mingw/lib/

    LIBS += -lavcodec.dll  -lavformat.dll -lswscale.dll

    ffmpegLib.files =       \
        $$PWD/ffmpeg/mingw/bin/avcodec*.dll \
        $$PWD/ffmpeg/mingw/bin/avformat*.dll \
        $$PWD/ffmpeg/mingw/bin/swscale*.dll  \
        $$PWD/ffmpeg/mingw/bin/avutil*.dll  \
        $$PWD/ffmpeg/mingw/bin/swresample*.dll
}

msvc: {
    INCLUDEPATH  +=$$PWD/ffmpeg/vs/include
    LIBS += -L$$PWD/ffmpeg/vs/lib/

    LIBS += -lavcodec  -lavformat -lswscale
    ffmpegLib.files =       \
        $$PWD/ffmpeg/vs/bin/avcodec*.dll \
        $$PWD/ffmpeg/vs/bin/avformat*.dll \
        $$PWD/ffmpeg/vs/bin/swscale*.dll  \
        $$PWD/ffmpeg/vs/bin/avutil*.dll   \
        $$PWD/ffmpeg/vs/bin/swresample*.dll
}

ffmpegLib.path = $$DESTDIR

INSTALLS += ffmpegLib

HEADERS += \
    $$PWD/imageConvert.h

SOURCES += \
    $$PWD/imageConvert.cpp
