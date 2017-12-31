#ifndef JAIGIGECAMERA_H
#define JAIGIGECAMERA_H

#include <stdint.h>
#include <QList>
#include <QString>

class GigeCameraBasePrivate;

typedef enum JaiFrameType {
    JaiFrameGray8Bit,
    JaiFrameGray10BitOCCU2ByteMSB,
    JaiFrameGray12BitOCCU2ByteMSB,
    JaiFrameRGB888,
}JaiFrameType;

typedef struct JaiFrameTag {
    long nWidth;
    long nHeight;
    long nStamp;
    long dwFrameNum;
    int  nType;
    QByteArray data;
}JaiGSFrame;

class JaiGigeCamera
{
public:
    JaiGigeCamera();
    ~JaiGigeCamera();
    bool closeCamera();
    static QStringList getDeviceList(QStringList &outIdList);
    void dumpCameraCategoryNode();
    bool openCamera(QString cameraID);
    bool configCamera();
    bool startPlayStream(int streamNum, bool useHistogram = false);
    bool stopPlayStream();
    bool getFrame(JaiGSFrame& frame, QList<int>* redHis = NULL, QList<int>* greenHis = NULL, QList<int>* blueHis = NULL);
    bool setSize(int width, int height);
    bool setPixelFormat(QString pixelFormat);
    bool getGain(int &minGain, int &maxGain, int &currentGain);
    bool setGain(int gain);
    bool getExposureTime(double &minExposure, double &maxExposure, double &exposure);
    bool setExposureTime(double exposure);
private:
    bool setFramegrabberValue(const QString name, int64_t int64Val);
    bool SetFramegrabberPixelFormat(QString name, int64_t jaiPixelFormat);

private:
    QString cameraID;
    GigeCameraBasePrivate* p_d;
};

#endif // JAIGIGECAMERA_H
