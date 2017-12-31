#ifndef BASLERCAMERA_H
#define BASLERCAMERA_H

#include "baslercamera_global.h"
#include <QThread>

#define BUFFER_WIDTH  1024
#define BUFFER_HEIGHT 768
#define BUFFERSIZE  ((BUFFER_WIDTH)*(BUFFER_HEIGHT))
#define BUFF_RGB_SIZE   (BUFFERSIZE *3)

struct BalanceRatio{
    double R;
    double G;
    double B;
};

class BASLERCAMERASHARED_EXPORT BaslerCamera : public QThread
{
public:
    BaslerCamera();
    virtual ~BaslerCamera();

    void	run();
    void	stopPlay();
    bool    getFrame(unsigned char *p, int size);
    void    setCurrentExposureTime(int time);
    void    setBalanceRatio(double r,double g,double b);
    void    setPixelFormat(int index);
    bool    ConvertImageFromBayerBG8ToRGB24(unsigned char* pBuf,unsigned char* pBmp,unsigned int nWidth, unsigned int nHeight);

protected:
    int                 PixelFormat;
    bool                ContinueFlag;
    int                 VideoWidth;
    int                 VideoHeight;
    int                 CurrentExposureTime;
    BalanceRatio        BSRrgb;

private:
    unsigned char       GrabOriginImage[BUFFERSIZE];
    unsigned char       GrabRGBImage[BUFFERSIZE*3];
};

#endif // BASLERCAMERA_H
