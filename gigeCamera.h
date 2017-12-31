#ifndef GIGECAMERA_H
#define GIGECAMERA_H

#include <stdint.h>
#include <QList>
#include <QString>

class GigeCameraBasePrivate;

typedef enum JaiFrameType {
    JaiFrameGray,
    JaiFrameRGB888,
}JaiFrameType;

typedef struct GSFrameTag {
    long nWidth;
    long nHeight;
    long nStamp;
    long dwFrameNum;
    int  nType;
    QByteArray data;
}JaiGSFrame;

class GigeCamera
{
public:
    GigeCamera();
    ~GigeCamera();
    bool closeCamera();
    static QStringList getDeviceList(QStringList &outIdList);
    void dumpCameraCategoryNode();
    bool openCamera(QString cameraID);
    bool configCamera();
    bool startPlayStream(int streamNum, bool useHistogram = false);
    bool stopPlayStream();
    bool getFrame(JaiGSFrame& frame, QList<int>* redHis = NULL, QList<int>* greenHis = NULL, QList<int>* blueHis = NULL);
    bool setSize(int width, int height);

    bool getGain(int &minGain, int &maxGain, int &currentGain);
    bool setGain(int gain);
    bool getExposureTime(int &minExposure, int &maxExposure, int &exposure);
    bool setExposureTime(int exposure);
private:
    bool setFramegrabberValue(const QString name, int64_t int64Val);
    bool SetFramegrabberPixelFormat(QString name, int64_t jaiPixelFormat);

private:
    QString cameraID;
    GigeCameraBasePrivate* p_d;
};

#endif // GIGECAMERA_H
