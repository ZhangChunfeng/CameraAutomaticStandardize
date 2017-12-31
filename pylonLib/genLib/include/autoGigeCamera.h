#ifndef AUTOGIGECAMERA_H
#define AUTOGIGECAMERA_H

#include <QObject>
#include <QTimer>
#include <QtCore/qglobal.h>

#include "gigeCamera.h"

#if defined(GENPYLONGIGELIB_LIBRARY)
#  define GENGIGELIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define GENGIGELIBSHARED_EXPORT Q_DECL_IMPORT
#endif

class InnerData;

class GENGIGELIBSHARED_EXPORT AutoGigeCamera : public GigeCamera
{
public:
    explicit AutoGigeCamera();
    ~AutoGigeCamera();
    static void initPylonSdk();      // 必须先调用
    static void releasePylonSdk();   // 关闭程序时调用
    static QStringList getDeviceList(QStringList &outIdList);
    static void setGenICamLogConfig(const QString& path);
    static void setGenICamCacheFolder(const QString& path);
    bool startPlay(bool isYUVOnly = false);
    bool stopPlay();
    bool setImageSize(int width, int height);
    bool getFrameWithExpAndGain(BaslerFrame& frame, int &exposure, int &gain);
    bool setWorkModeSeq(bool isSeq);
    bool isSeqWorkMode();
    bool loadParamFromCamera();
    bool configDisplayParam(const SeqParam& param);
    bool configMeasurementParam(const QList<SeqParam>& params);
    bool getDisplayParam(SeqParam &param);
    bool getMeasurementParams(QList<SeqParam> &params);
private:
    InnerData*  data;
};

#endif // AUTOGIGECAMERA_H
