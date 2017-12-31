#include <QDebug>
#include "autoGigeCamera.h"
#include "gigeCamera.h"

class InnerData
{
    friend class AutoGigeCamera;
public:
    InnerData()
        : autoCamera(NULL)
        , isPlaying(false)
        , isSeqMode(false) {
    }

private:
    AutoGigeCamera* autoCamera;
    bool        isPlaying;
    bool        isSeqMode;
    QList<SeqParam> seqParams;
    SeqParam        dispParams;
};

AutoGigeCamera::AutoGigeCamera()
    : data(new InnerData)
{
}

AutoGigeCamera::~AutoGigeCamera()
{
    delete data;
}

void AutoGigeCamera::initPylonSdk()
{
    GigeCamera::initPylonSdk();
}

void AutoGigeCamera::releasePylonSdk()
{
    GigeCamera::releasePylonSdk();
}

QStringList AutoGigeCamera::getDeviceList(QStringList &outIdList)
{
    return GigeCamera::getDeviceList(outIdList);
}

void AutoGigeCamera::setGenICamLogConfig(const QString &path)
{
    return GigeCamera::setGenICamLogConfig(path);
}

void AutoGigeCamera::setGenICamCacheFolder(const QString &path)
{
    return GigeCamera::setGenICamCacheFolder(path);
}



bool AutoGigeCamera::startPlay(bool isYUVOnly)
{
    setPixelFormat("YUV422_YUYV_Packed");
    data->isPlaying = startPlayStream(isYUVOnly);
    return data->isPlaying;
}

bool AutoGigeCamera::stopPlay()
{
    data->isPlaying = false;
    return stopPlayStream();
}


bool AutoGigeCamera::getFrameWithExpAndGain(BaslerFrame &frame, int& exposure, int& gain)
{
    if (getFrame(frame)) {
        int idx = frame.seqIndex;
        if (idx >= 0) {
            if (((idx & 0x1) == 0)) {
                exposure = data->dispParams.exposureTime;
                gain = data->dispParams.gain;
            } else if (data->isSeqMode) {
                idx = idx >> 1;
                if (idx < data->seqParams.count()) {
                    exposure = data->seqParams.at(idx).exposureTime;
                    gain = data->seqParams.at(idx).gain;
                } else {
                    //                qWarning()<<"error......";
                }
            }
        }
        return true;
    }
    return false;
}

bool AutoGigeCamera::setImageSize(int width, int height)
{
    bool ret = false;
    stopPlayStream();
    bool isSeq = data->isSeqMode;
    setWorkModeSeq(false);
    if (setSize(width, height)) {
        ret = true;
    }
    setWorkModeSeq(isSeq);
    return ret;
}

bool AutoGigeCamera::setWorkModeSeq(bool isSeq)
{
    stopPlayStream();
    setSequenceEnable(false);

    if (isSeq) {
        if (data->seqParams.isEmpty()) {
            qWarning()<<"no seq config, please config before setMode";
            return false;
        }
        QList<SeqParam> list;
        foreach (const SeqParam& p, data->seqParams) {
            list<<data->dispParams<<p;
        }
        int count = list.count();
        setSequenceCount(count);
        for (int i = 0; i < count; i++) {
            SeqParam param = list.at(i);
            if (param.frameCount <= 0) {
                continue;
            }
            setSequenceIndex(i);
            setGain(param.gain);
            setExposureTime(param.exposureTime);
            setSequenceExcutionCnt(param.frameCount);
            if (param.isDiaplay) {
                setPixelFormat("YUV422_YUYV_Packed");
            } else {
                setPixelFormat("Mono8");
            }
            enableChunkForSequnce(true);
            setAutoExposureMode("Off");
            execSaveSetSeq();
        }
        setSequenceEnable(true);
    } else {
        setGain(data->dispParams.gain);
        setExposureTime(data->dispParams.exposureTime);
        setPixelFormat("YUV422_YUYV_Packed");
    }
    if (data->isPlaying) {
        startPlay();
    }
    data->isSeqMode = isSeq;
    qDebug()<<"isSeq is "<<isSeq<<data->isPlaying;
    return true;
}

bool AutoGigeCamera::isSeqWorkMode()
{
    return data->isSeqMode;
}

bool AutoGigeCamera::loadParamFromCamera()
{
    stopPlayStream();
    getSequenceEnable(data->isSeqMode);
    setSequenceEnable(false);

    int count;
    getSequenceCount(count);
    data->seqParams.clear();
    for (int i = 0; i < count; i++) {
        setSequenceIndex(i);
        execLoadSeqSet();
        SeqParam p;
        int min, max;
        getExposureTime(min, max, p.exposureTime);
        getSequenceExcutionCnt(p.frameCount);
        getGain(min, max, p.gain);
        p.isDiaplay = (i & 0x1) ? false : true;;
        if (p.isDiaplay) {
            data->dispParams = p;
            continue;
        } else {
            data->seqParams.append(p);
        }
    }

    if (count <= 0) {
        SeqParam p;
        int min, max;
        getExposureTime(min, max, p.exposureTime);
        getSequenceExcutionCnt(p.frameCount);
        getGain(min, max, p.gain);
        p.isDiaplay = true;
        data->dispParams = p;
    }
    setSequenceEnable(data->isSeqMode);

    return true;
}

bool AutoGigeCamera::configDisplayParam(const SeqParam &param)
{
    data->dispParams = param;
    return setWorkModeSeq(data->isSeqMode);
}

bool AutoGigeCamera::configMeasurementParam(const QList<SeqParam>& params)
{
    data->seqParams = params;
    return setWorkModeSeq(data->isSeqMode);
}

bool AutoGigeCamera::getDisplayParam(SeqParam &param)
{
    param = data->dispParams;
    return true;
}

bool AutoGigeCamera::getMeasurementParams(QList<SeqParam> &params)
{
    params = data->seqParams;
    return true;
}

