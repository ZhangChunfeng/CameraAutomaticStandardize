#ifndef GIGECAMERA_H
#define GIGECAMERA_H

#include <QList>
#include <QString>

#include "cameraTypedef.h"

#if defined(GENPYLONGIGELIB_LIBRARY)
#  define GENGIGELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define GENGIGELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

class GigeCameraPrivate;

class GENGIGELIBSHARED_EXPORT GigeCamera
{
public:
    GigeCamera();
    ~GigeCamera();
    static void initPylonSdk();      // 必须先调用
    static void releasePylonSdk();   // 关闭程序时调用
    static QStringList getDeviceList(QStringList &outIdList);
    static void setGenICamLogConfig(const QString& path);
    static void setGenICamCacheFolder(const QString& path);
    QString getCamerID();
    bool isOnLine();
    bool loadCameraConfig(QString filePath);
    bool saveCameraConfig(QString filePath);

    bool openCamera(QString cameraID);
    bool closeCamera();
    bool startPlayStream(bool isYUVOnly = false);
    bool stopPlayStream();
    bool getFrame(BaslerFrame& frame);
    bool getMaxMinSize(int& minWidth, int& minHeight, int &maxWidth, int& maxHeight);
    bool getSize(int& width, int& height);
    bool setSize(int width, int height);
    bool getSequenceEnable(bool& enable);
    bool setSequenceEnable(bool enable);
    bool setSequenceCount(int seqCount);
    bool getSequenceCount(int& seqCount);
    bool setSequenceIndex(int index);
    bool setSequenceExcutionCnt(int count);
    bool getSequenceExcutionCnt(int& count);
    bool getGain(int &minGain, int &maxGain, int &currentGain);
    bool setGain(int gain);
    bool getExposureTime(int &minExposure, int &maxExposure, int &exposure);
    bool setExposureTime(int exposure);
    bool execLoadSeqSet();
    bool execSaveSetSeq();
    bool setAutoExposureMode(QString mode); // Off - 自动曝光关闭， Once - 自动曝光一次， Continuous - 连续自动曝光
    bool enableChunkForSequnce(bool enable);
    bool setPixelFormat(QString format);
    QString getPixelFormat();

    void setRawImageHandler(RawImageArrivedHander handler, void *userPtr);
private:
    QString cameraID;
    GigeCameraPrivate* p_d;
};

#endif // GIGECAMERA_H
