#ifndef MYTYPEDEF_H
#define MYTYPEDEF_H

#include <QByteArray>

typedef enum FrameType {
    FrameGray,
    FrameYUV420,
    FrameRGB888,
}FrameType;

typedef struct BaslerFrameTag {
    long nWidth;
    long nHeight;
    long nStamp;
    long dwFrameNum;
    int  nType;
    int  seqIndex;
    float fps;
    QByteArray data;
}BaslerFrame;

typedef struct SeqParamTag {
    int     exposureTime;
    int     gain;
    int     frameCount;
    bool    isDiaplay;
    SeqParamTag() {
        exposureTime = 0;
        gain = 0;
        frameCount = 0;
        isDiaplay = false;
    }
}SeqParam;

typedef void (*RawImageArrivedHander)(BaslerFrame& frame, void* userPtr);

class GigeCamera;

#endif // MYTYPEDEF_H
