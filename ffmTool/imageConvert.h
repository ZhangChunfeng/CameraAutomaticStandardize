#ifndef IMAGECONVERT_H
#define IMAGECONVERT_H

//#include <QImage>
bool YV12_to_RGB24(
                   /*in*/int pm_width,
                   /*in*/int pm_height,
                   /*in*/unsigned char* pm_pBufYV12,
                   /*out*/unsigned char* pm_pBufRGB24);
bool YV12ToBGR24_Native(int width,int height, unsigned char* pYUV, unsigned char* pBGR24);
//bool YV12ToBGR24_FFmpeg(int width,int height, unsigned char* pYUV,unsigned char* pBGR24);
//bool YV12ToBGR32_FFmpeg(int width, int height, unsigned char* pYUV, int w, int h, unsigned char* pBGR32);
bool YV12ToRGB32_FFmpeg(int width, int height, unsigned char* pYUV, int w, int h, unsigned char* pRBG32);

#endif // IMAGECONVERT_H

