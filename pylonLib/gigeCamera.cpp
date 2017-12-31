#include <QDebug>
#include <QTime>
#include <QFile>
#include <QTimer>
#include <QReadLocker>
#include <QElapsedTimer>
#include <pylon/PylonIncludes.h>
#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/BaslerGigEInstantCamera.h>
#include <pylon/gige/BaslerGigEGrabResultData.h>
#include <pylon/ImageFormatConverter.h>
#include "gigeCamera.h"
#include "base/GCUtilities.h"
#include "help/ConfigurationEventPrinter.h"
#include "help/CameraEventPrinter.h"

using namespace Pylon;
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
typedef Pylon::CBaslerGigEImageEventHandler ImageEventHandler_t;
typedef Pylon::CBaslerGigEGrabResultPtr GrabResultPtr_t;

#define METERING_RADIUS_PEXEL   5
using namespace Basler_GigECameraParams;
using namespace GenApi;

class GigeCameraPrivate : public QObject, public CImageEventHandler, public CConfigurationEventPrinter, public CCameraEventPrinter
{
public:
    friend class GigeCamera;
    void OnImageGrabbed(CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult);
    void OnImagesSkipped(CInstantCamera& camera, size_t countOfSkippedImages)
    {
        qWarning() << "OnImagesSkipped event for device " << camera.GetDeviceInfo().GetModelName();
        qWarning() << countOfSkippedImages  << " images have been skipped.";
    }
    void OnCameraEvent( CInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode)
    {
        qWarning() << "OnCameraEvent event for device " << camera.GetDeviceInfo().GetModelName();
        qWarning() << "User provided ID: " << userProvidedId;
        qWarning() << "Event data node name: " << pNode->GetName();
        GenApi::CValuePtr ptrValue( pNode );
        if ( ptrValue.IsValid() ) {
            qWarning() << "Event node data: " << ptrValue->ToString();
        }
        qWarning()<<"";
    }
    GigeCameraPrivate()
        : frameCount(0)
        , fps(0.0)
        , rawImageHander(NULL)
        , userPtr(NULL)
        , isYuvOnly(false) {
        for (int i = 0; i < 10; i++) {
            emptyFrames.append(new BaslerFrame);
        }
        connect(&checkTimer, &QTimer::timeout, this, &GigeCameraPrivate::checkTimerOut);
    }
    ~GigeCameraPrivate() {
        qDeleteAll(fullFrames);
        qDeleteAll(emptyFrames);
        fullFrames.clear();
        emptyFrames.clear();
        camera.DestroyDevice();
    }
    void startCheck() {
        checkCount = 0;
        if (!checkTimer.isActive()) {
            checkTimer.start(1000);
        }
    }

    void stopCheck() {
        checkCount = 0;
        if (checkTimer.isActive()) {
            checkTimer.stop();
        }
    }
    bool stashConfig();
    bool restoreConfig();

    bool openCamera(QString cameraID);
    bool closeCamera();
    bool startPlayStream();
    bool stopPlayStream();
private slots:
    void checkTimerOut();

private:
    QElapsedTimer   t;
    QList<BaslerFrame*> fullFrames;
    QList<BaslerFrame*> emptyFrames;
    QByteArray      imageBuff;
    BaslerFrame         rawFrame;
    int             frameCount;
    float           fps;
    QReadWriteLock  lock;
private:
    Camera_t camera;
    CImageFormatConverter imageConvert;
    QByteArray  grayData;
    RawImageArrivedHander  rawImageHander;
    void*                  userPtr;
private:
    // 考虑到数据转换消耗CPU资源的问题，当主界面不显示时，
    // 只输出yuv格式图像直接用于后续H264编码减轻cpu负担
    bool            isYuvOnly;
    int             checkCount;
    QTimer          checkTimer;
    QString         cameraID;
    String_t        configrations;
};

void GigeCamera::initPylonSdk()
{
    PylonInitialize();
}

void GigeCamera::releasePylonSdk()
{
    PylonTerminate();
}

GigeCamera::GigeCamera()
    : p_d(new GigeCameraPrivate)
{
}

GigeCamera::~GigeCamera()
{
    closeCamera();
    delete p_d;
}

QStringList GigeCamera::getDeviceList(QStringList& outIdList)
{
    QStringList modelNameList;
    CTlFactory& TlFactory = CTlFactory::GetInstance();
    DeviceInfoList_t lstDevices;
    TlFactory.EnumerateDevices( lstDevices );
    if (!lstDevices.empty()) {
        DeviceInfoList_t::const_iterator it;
        for ( it = lstDevices.begin(); it != lstDevices.end(); ++it ) {
            outIdList << QString(it->GetFullName().c_str());
            qDebug() << QString(it->GetFullName().c_str());
            qDebug() << QString(it->GetSerialNumber().c_str());
            qDebug() << QString(it->GetDeviceClass().c_str());
            qDebug() << QString(it->GetDeviceFactory().c_str());
            qDebug() << QString(it->GetDeviceVersion().c_str());
            qDebug() << QString(it->GetModelName().c_str());
            modelNameList<<QString(it->GetModelName().c_str());
        }
    }
    return modelNameList;
}

void GigeCamera::setGenICamLogConfig(const QString &path)
{
    gcstring p(path.toLatin1().data());
    SetGenICamLogConfig(p);
}

void GigeCamera::setGenICamCacheFolder(const QString &path)
{
    gcstring p(path.toLatin1().data());
    SetGenICamCacheFolder(p);
}

QString GigeCamera::getCamerID()
{
    return cameraID;
}

bool GigeCamera::isOnLine()
{
    return !p_d->camera.IsCameraDeviceRemoved();
}

static void myConvertYUYV422Packeted2YUV420(unsigned char* dest, int destSize, int w, int h, unsigned char* src, int srcSize)
{
    unsigned char* y, *u, *v;
    Q_ASSERT(destSize == ((w * h * 3) >> 1));
    Q_ASSERT(srcSize == ((w * h) << 1));
    y = dest;
    u = dest + w * h;
//    v = u + ((w * h) >> 2);

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            *y++ = *src++;
            if (i & 0x1) {
                src++;
            } else {
//                if (j & 0x1) {
//                    *v++ = *src++;
//                } else {
                    *u++ = *src++;
//                }
            }
        }
    }
}

static void myConvertYUV422Packeted2YUV420(unsigned char* dest, int destSize, int w, int h, unsigned char* src, int srcSize)
{
    unsigned char* y, *u, *v;
    Q_ASSERT(destSize == ((w * h * 3) >> 1));
    Q_ASSERT(srcSize == ((w * h) << 1));
    y = dest;
    u = dest + w * h;
//    v = u + ((w * h) >> 2);

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (i & 0x1) {
                src++;
            } else {
//                if (j & 0x1) {
//                    *v++ = *src++;
//                } else {
                    *u++ = *src++;
//                }
            }
            *y++ = *src++;
        }
    }
}

void GigeCameraPrivate::OnImageGrabbed(CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
{
    Q_UNUSED(camera);
//    qDebug() << "OnImageGrabbed event for device " << camera.GetDeviceInfo().GetModelName();
    if (ptrGrabResult->GrabSucceeded()) {
        int width  = ptrGrabResult->GetWidth();
        int height = ptrGrabResult->GetHeight();

        // Check to see if a buffer containing chunk data has been received.
//        if (PayloadType_ChunkData != ptrGrabResult->GetPayloadType()) {
//            throw RUNTIME_EXCEPTION( "Unexpected payload type received.");
//        }
        CBaslerGigEGrabResultData* res = (CBaslerGigEGrabResultData*)ptrGrabResult.operator ->();
        int seqIndex = -1;
        if (GenApi::IsReadable(res->ChunkSequenceSetIndex)) {
            seqIndex = res->ChunkSequenceSetIndex.GetValue();
        }
        rawFrame.seqIndex = seqIndex;
        rawFrame.fps = fps;
        rawFrame.dwFrameNum = res->GetImageNumber();
        rawFrame.nStamp = res->GetTimeStamp();
        rawFrame.nWidth = width;
        rawFrame.nHeight = height;
        int frameType = FrameRGB888;
        EPixelType pixelType = res->GetPixelType();
        if (pixelType == PixelType_Mono8) { // 黑白图像
            size_t size = ptrGrabResult->GetImageSize();
            if (imageBuff.size() != (int)size) {
                imageBuff.resize(size);
            }
            memcpy(imageBuff.data(), ptrGrabResult->GetBuffer(), size);
            frameType = FrameGray;
        } else {
            if (!isYuvOnly) {
                size_t size = imageConvert.GetBufferSizeForConversion(ptrGrabResult);
                if (imageBuff.size() != (int)size) {
                    imageBuff.resize(size);
                }
                imageConvert.Convert(imageBuff.data(), size, ptrGrabResult);
            }
            frameType = FrameRGB888;
            if (((pixelType == PixelType_YUV422_YUYV_Packed) || (pixelType == PixelType_YUV422packed)) &&  rawImageHander) {
                int yuvSize = width * height * 2;
                if (rawFrame.data.size() != (int)yuvSize) {
                    rawFrame.data.resize(yuvSize);
                }
                rawFrame.nType = FrameYUV420;
//                if (pixelType == PixelType_YUV422_YUYV_Packed) {
//                    myConvertYUYV422Packeted2YUV420((unsigned char*)rawFrame.data.data(), yuvSize, width, height,
//                                                (unsigned char*)res->GetBuffer(), res->GetImageSize());
//                } else {
//                    myConvertYUV422Packeted2YUV420((unsigned char*)rawFrame.data.data(), yuvSize, width, height,
//                                                (unsigned char*)res->GetBuffer(), res->GetImageSize());
//                }
                memcpy(rawFrame.data.data(), ptrGrabResult->GetBuffer(), yuvSize);
                rawImageHander(rawFrame, userPtr);
            }
        }

        frameCount++;
        qreal ms = t.elapsed();
        if (ms > 1000) {
            t.restart();
            fps = frameCount * 1000 / ms;
//            qWarning()<<"fps: "<<fps;
            frameCount = 0;
        }
        lock.lockForWrite();
        if ((!isYuvOnly) && (!emptyFrames.isEmpty())) {
            BaslerFrame* frame = emptyFrames.takeFirst();
            frame->seqIndex = seqIndex;
            frame->fps = fps;
            frame->dwFrameNum = rawFrame.dwFrameNum;
            frame->nStamp = rawFrame.nStamp;
            frame->nWidth = width;
            frame->nHeight = height;
            frame->nType = frameType;
            frame->data.swap(imageBuff);
            fullFrames.append(frame);
        }
        lock.unlock();
    } else {
        qWarning() << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() ;
    }
}

bool GigeCamera::openCamera(QString cameraID)
{
    return p_d->openCamera(cameraID);
}

bool GigeCamera::closeCamera()
{
    return p_d->closeCamera();
}

bool GigeCamera::startPlayStream(bool isYUVOnly)
{
    p_d->isYuvOnly = isYUVOnly;
    p_d->stashConfig();
    bool ret = p_d->startPlayStream();
    p_d->startCheck();
    return ret;
}

bool GigeCamera::stopPlayStream()
{
    p_d->stopCheck();
    return p_d->stopPlayStream();;
}

bool GigeCamera::getFrame(BaslerFrame &frame)
{
    bool ret = false;
    if (p_d->lock.tryLockForWrite()) {
        if (!p_d->fullFrames.isEmpty()) {
            BaslerFrame* f = p_d->fullFrames.takeFirst();
            frame = *f;
            p_d->emptyFrames.append(f);
            ret = true;
        }
        p_d->lock.unlock();
    }
    return ret;
}

bool GigeCamera::getMaxMinSize(int &minWidth, int &minHeight, int &maxWidth, int &maxHeight)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsReadable(p_d->camera.Width)) {
        minWidth = p_d->camera.Width.GetMin();
        maxWidth = p_d->camera.Width.GetMax();
    } else {
        qWarning()<<"cannot get width, node is unReadable.";
        return false;
    }
    if (GenApi::IsReadable(p_d->camera.Height)) {
        minHeight = p_d->camera.Height.GetMin();
        maxHeight = p_d->camera.Height.GetMax();
    } else {
        qWarning()<<"cannot get height, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::getSize(int &width, int &height)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CIntegerPtr node(nodemap.GetNode("Width"));
    if (GenApi::IsReadable(node)) {
        width = node->GetValue();
    } else {
        qWarning()<<"cannot get width, node is unReadable.";
        return false;
    }
    CIntegerPtr node2(nodemap.GetNode("Height"));
    if (GenApi::IsReadable(node2)) {
        height = node2->GetValue();
    } else {
        qWarning()<<"cannot get Height, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::setSize(int width, int height)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (width % 4 != 0) {
        qWarning()<<"width must be multiple 4!, input width is "<<width;
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.Width)) {
        p_d->camera.Width.SetValue(width);
    } else {
        qWarning()<<"cannot set width, node is unWriteable.";
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.Height)) {
        p_d->camera.Height.SetValue(height);
    } else {
        qWarning()<<"cannot set height, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::getSequenceEnable(bool &enable)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsReadable(p_d->camera.SequenceEnable)) {
        enable = p_d->camera.SequenceEnable.GetValue();
    } else {
        qWarning()<<"cannot set SequenceEnable, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::setSequenceEnable(bool enable)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.SequenceEnable)) {
        p_d->camera.SequenceEnable.SetValue(enable);
    } else {
        qWarning()<<"cannot set SequenceEnable, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::setSequenceCount(int seqCount)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.SequenceSetTotalNumber)) {
        p_d->camera.SequenceSetTotalNumber.SetValue(seqCount);
    } else {
        qWarning()<<"cannot set SequenceSetTotalNumber, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::getSequenceCount(int &seqCount)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CIntegerPtr seqSetCount(nodemap.GetNode("SequenceSetTotalNumber"));
    if (GenApi::IsReadable(seqSetCount)) {
        seqCount = seqSetCount->GetValue();
    } else {
        qWarning()<<"cannot set SequenceSetTotalNumber, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::setSequenceIndex(int index)
{
    int count;
    if (!getSequenceCount(count)) {
         return false;
    }

    if ((index >= count) || (index < 0)) {
        qWarning()<<QString("the sequenceIndex out of the range (%1,%2)").arg(0).arg(count);
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CIntegerPtr seqIndex(nodemap.GetNode("SequenceSetIndex"));
    if (GenApi::IsWritable(seqIndex)) {
        seqIndex->SetValue(index);
    } else {
        qWarning()<<"cannot set SequenceSetIndex, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::setSequenceExcutionCnt(int count)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.SequenceSetExecutions)) {
        p_d->camera.SequenceSetExecutions.SetValue(count);
    } else {
        qWarning()<<"cannot set SequenceSetExecutions, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::getSequenceExcutionCnt(int &count)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsReadable(p_d->camera.SequenceSetExecutions)) {
        count = p_d->camera.SequenceSetExecutions.GetValue();
    } else {
        qWarning()<<"cannot set SequenceSetExecutions, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::getGain(int& minGain, int& maxGain, int& currentGain)
{
    // Get the camera control object.
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    // Access the GainRaw integer type node. This node is available for IIDC 1394 and GigE camera devices.
    CIntegerPtr gainRaw(nodemap.GetNode("GainRaw"));
    if (GenApi::IsReadable(gainRaw)) {
        minGain = gainRaw->GetMin();
        maxGain = gainRaw->GetMax();
        currentGain = gainRaw->GetValue();
    } else {
        qWarning()<<"cannot get gain, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::setGain(int gain)
{
    int minGain, maxGain, currentGain;
    if (!getGain(minGain, maxGain, currentGain)) {
         return false;
    }

    if ((gain > maxGain) || (gain < minGain)) {
        qWarning()<<QString("the gain %1 out of the range (%1,%2)")
                    .arg(gain).arg(minGain).arg(maxGain);
        return false;
    }

    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CIntegerPtr gainRaw(nodemap.GetNode("GainRaw"));
    if (GenApi::IsWritable(gainRaw)) {
        gainRaw->SetValue(gain);
    } else {
        qWarning()<<"cannot set gain, node is unWriteable.";
        return false;
    }

    return true;
}

bool GigeCamera::getExposureTime(int& minExposure, int& maxExposure, int& exposure)
{
    // Get the camera control object.
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    // Access the GainRaw integer type node. This node is available for IIDC 1394 and GigE camera devices.
    CIntegerPtr exposureTime(nodemap.GetNode("ExposureTimeRaw"));
    if (GenApi::IsReadable(exposureTime)) {
        minExposure = exposureTime->GetMin();
        maxExposure = exposureTime->GetMax();
        exposure = exposureTime->GetValue();
    } else {
        qWarning()<<"cannot get exposureTimeRaw, node is unReadable.";
        return false;
    }
    return true;
}

bool GigeCamera::setExposureTime(int exposure)
{
    int minExposure, maxExposure, currentExposure;
    if (!getExposureTime(minExposure, maxExposure, currentExposure)) {
         return false;
    }

    if ((exposure > maxExposure) || (exposure < minExposure)) {
        qWarning()<<QString("the ExposureTime %1 out of the range (%1,%2)")
                    .arg(exposure).arg(minExposure).arg(maxExposure);
        return false;
    }

    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CIntegerPtr exposureTime(nodemap.GetNode("ExposureTimeRaw"));
    if (GenApi::IsWritable(exposureTime)) {
        exposureTime->SetValue(exposure);
    } else {
        qWarning()<<"cannot set exposureTimeRaw, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::execLoadSeqSet()
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.SequenceSetLoad)) {
        p_d->camera.SequenceSetLoad.Execute();
    } else {
        qWarning()<<"cannot exec SequenceSetLoad, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::execSaveSetSeq()
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    if (GenApi::IsWritable(p_d->camera.SequenceSetStore)) {
        p_d->camera.SequenceSetStore.Execute();
    } else {
        qWarning()<<"cannot exec SequenceSetStore, node is unWriteable.";
        return false;
    }
    return true;
}

bool GigeCamera::setAutoExposureMode(QString mode)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }

    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CEnumerationPtr node(nodemap.GetNode("ExposureAuto"));
    GenICam::gcstring s(mode.toLatin1().data());
    if (GenApi::IsAvailable(node->GetEntryByName(s))) {
        if (GenApi::IsWritable(node)) {
            node->FromString(s);
            return true;
        } else {
            qWarning()<<"cannot set AutoExposureMode, AutoExposureMode node is unWriteable. ";
        }
    } else {
        qWarning()<<"cannot set AutoExposureMode, AutoExposureMode is unAvailable. "<<mode;
    }
    return false;
}

bool GigeCamera::enableChunkForSequnce(bool enable)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CBooleanPtr node(nodemap.GetNode("ChunkModeActive"));
    if (GenApi::IsWritable(node)) {
        node->SetValue(enable);
    } else {
        qWarning()<<"cannot set ChunkModeActive, node is unWriteable.";
        return false;
    }

    if (enable) {
        // Enable time stamp chunks.
        p_d->camera.ChunkSelector.SetValue(ChunkSelector_SequenceSetIndex);
        p_d->camera.ChunkEnable.SetValue(true);
    }
    return true;
}

bool GigeCamera::setPixelFormat(QString format)
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CEnumerationPtr pixelFormat(nodemap.GetNode("PixelFormat"));
    // Set the pixel format to Mono8 if available.
    GenICam::gcstring s(format.toLatin1().data());
    if (GenApi::IsAvailable(pixelFormat->GetEntryByName(s))) {
        if (GenApi::IsWritable(pixelFormat)) {
            pixelFormat->FromString(s);
            qDebug() << "New PixelFormat  : " << pixelFormat->ToString();
            return true;
        } else {
            qWarning()<<"cannot set PixelFormat, PixelFormat node is unWriteable. ";
        }
    } else {
        qWarning()<<"cannot set PixelFormat, PixelFormat is unAvailable. "<<format;
    }
    return false;
}

QString GigeCamera::getPixelFormat()
{
    if (!p_d->camera.IsOpen()) {
        return false;
    }
    INodeMap &nodemap = p_d->camera.GetNodeMap();
    CEnumerationPtr pixelFormat(nodemap.GetNode("PixelFormat"));
    // Set the pixel format to Mono8 if available.
    QString format("");
    if (GenApi::IsReadable(pixelFormat)) {
        GenICam::gcstring s = pixelFormat->ToString();
        format = QString("%1").arg(s.c_str());
    } else {
        qWarning()<<"cannot read PixelFormat, PixelFormat node is unReadable. ";
    }
    return format;
}

void GigeCamera::setRawImageHandler(RawImageArrivedHander handler, void* userPtr)
{
    p_d->lock.lockForWrite();
    p_d->rawImageHander = handler;
    p_d->userPtr = userPtr;
    p_d->lock.unlock();
}

bool GigeCamera::loadCameraConfig(QString filePath)
{
    try {
        QFile f(filePath);
        if (!f.open(QFile::ReadOnly)) {
            qDebug()<<QString("open file %1 error! ").arg(filePath)<<f.errorString();
            return false;
        }
        QByteArray s = f.readAll();
        f.close();
        qDebug()<<"config camera using "<<filePath;
        s.replace("\r\n", "\n");
        CFeaturePersistence::LoadFromString(s.data(), &p_d->camera.GetNodeMap(), true );
        return true;
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

bool GigeCamera::saveCameraConfig(QString filePath)
{
    try {
        QFile f(filePath);
        if (!f.open(QFile::WriteOnly)) {
            qDebug()<<QString("open file %1 error! ").arg(filePath)<<f.errorString();
            return false;
        }
        String_t dataStr;
        CFeaturePersistence::SaveToString(dataStr, &p_d->camera.GetNodeMap());
        f.write(dataStr.c_str(), dataStr.size());
        f.close();

        qDebug()<<"save configFile to "<<filePath;
        return true;
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}
bool GigeCameraPrivate::closeCamera()
{
    try {
        stopPlayStream();
        camera.DeregisterImageEventHandler(this);
        camera.DeregisterConfiguration(this);
//        p_d->camera.DeregisterCameraEventHandler(p_d, Cleanup_None);
        camera.Close();
        camera.DestroyDevice();
        camera.DetachDevice();
        qDebug()<<"close camera "<<cameraID;
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
    }
    return true;
}

bool GigeCameraPrivate::openCamera(QString cameraID)
{
    try {
        CDeviceInfo info;
        info.SetFullName(cameraID.toLatin1().data());
        camera.Attach(CTlFactory::GetInstance().CreateDevice(info));
        camera.Open();
        camera.RegisterImageEventHandler(this, RegistrationMode_ReplaceAll, Cleanup_None);
        camera.RegisterConfiguration(this, RegistrationMode_ReplaceAll, Cleanup_None);

        if (GenApi::IsReadable(camera.Width)) {
            int width = camera.Width.GetValue();
            if (width & 0x3) {
                if (GenApi::IsWritable(camera.Width)) {
                    camera.Width.SetValue(width & (~0x3));
                }
            }
        } else {
            qWarning()<<"cannot get width, node is unReadable.";
            return false;
        }

        qDebug()<<"open camera "<<cameraID;
        this->cameraID = cameraID;
        return camera.IsOpen();
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

bool GigeCameraPrivate::startPlayStream()
{
    try {
        imageConvert.OutputPixelFormat = PixelType_RGB8packed;
        camera.StartGrabbing( GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
        t.restart();
        qDebug()<<"start grabbing .";
        return camera.IsGrabbing();
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

bool GigeCameraPrivate::stopPlayStream()
{
    try {
        camera.StopGrabbing();
        t.invalidate();
        qDebug()<<"stop grabbing "<<!camera.IsGrabbing();
        return !camera.IsGrabbing();
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

bool GigeCameraPrivate::stashConfig()
{
    try {
        CFeaturePersistence::SaveToString(configrations, &camera.GetNodeMap());
        return true;
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

bool GigeCameraPrivate::restoreConfig()
{
    if (configrations.empty()) {
        return false;
    }
    try {
        CFeaturePersistence::LoadFromString(configrations, &camera.GetNodeMap(), true);
        return true;
    } catch (GenICam::GenericException &e) {
        qDebug() << "An exception occurred." << endl
                 << e.GetDescription() << endl;
        return false;
    }
}

void GigeCameraPrivate::checkTimerOut()
{
    if (camera.IsCameraDeviceRemoved()) {
        if (checkCount++ > 5) {
            checkCount = 0;
            QStringList list;
            GigeCamera::getDeviceList(list);
            if (list.contains(cameraID)) {
                closeCamera();
                if (openCamera(cameraID)) {
                    if (!restoreConfig()) {
                        qDebug()<<"restore configration failed. context: "<<configrations;
                    }
                    startPlayStream();
                }
            }
        }
    }
}
