#include <QDebug>
#include <QMutex>
#include <QElapsedTimer>
#include "jaiGigeCamera.h"
#include <qt_windows.h>
#include "jai_factory.h"
#include "Jai_Error.h"

#define NODE_NAME_WIDTH         (int8_t*)"Width"
#define NODE_NAME_HEIGHT        (int8_t*)"Height"
#define NODE_NAME_PIXELFORMAT   (int8_t*)"PixelFormat"
#define NODE_NAME_GAIN          (int8_t*)"GainRaw"
#define NODE_NAME_EXPOSURE      (int8_t*)"ExposureTime"
#define NODE_NAME_ACQSTART      (int8_t*)"AcquisitionStart"
#define NODE_NAME_ACQSTOP       (int8_t*)"AcquisitionStop"

class GigeSingleOnePrivate
{
public:
    GigeSingleOnePrivate():hFactory(NULL){}

    bool openFactory() {
        if (hFactory) {
            return true;
        }
        J_STATUS_TYPE retval;
        retval = J_Factory_Open((int8_t*)&"" , &hFactory);
        if (retval != J_ST_SUCCESS) {
            qDebug()<<"Could not open the factory!"<<retval;
            hFactory = NULL;
            return false;
        }
        return true;
    }

    QStringList getDeviceList(QStringList& cameraIdList) {
        QStringList list;
        J_STATUS_TYPE retval;
        bool8_t bHasChange;
        uint32_t nCameras;
        uint32_t size;
        int8_t sCameraId[J_CAMERA_ID_SIZE]; // Camera ID
        // Update camera list
        openFactory();
        retval = J_Factory_UpdateCameraList(hFactory, &bHasChange);
        if (retval != J_ST_SUCCESS) {
            qDebug()<<"Could not update the camera list!"<<retval;
            return list;
        }
        retval = J_Factory_GetNumOfCameras(hFactory, &nCameras);
        if (retval != J_ST_SUCCESS) {
            qDebug()<<"Could not get the number of cameras!"<<retval;
            return list;
        }
        if (nCameras == 0) {
            qDebug("There is no camera!\n");
            return list;
        }
        for (uint32_t i = 0; i < nCameras; i++) {
            size = sizeof(sCameraId);
            memset(sCameraId, 0, size);
            retval = J_Factory_GetCameraIDByIndex(hFactory, i, sCameraId, &size);
            if (retval != J_ST_SUCCESS) {
                qDebug()<<QString("Could not get the camera ID for index %1!").arg(i)<<retval;
                continue;
            }

            int8_t buffer[J_CAMERA_INFO_SIZE] = {0};
            size = sizeof(buffer);
            retval = J_Factory_GetCameraInfo(hFactory, sCameraId, CAM_INFO_MODELNAME, buffer, &size);
            if (retval != J_ST_SUCCESS) {
                qDebug()<<QString("Could not get the camera name for index %1!").arg(i)<<retval;
                continue;
            }
            cameraIdList<<QString().sprintf("%s", sCameraId);
            list<<QString().sprintf("%s", buffer);
        }
        return list;
    }

    ~GigeSingleOnePrivate() {
        if (hFactory) {
            J_STATUS_TYPE retval;
            try {
                retval = J_Factory_Close(hFactory);
            } catch (...){

            };
            qDebug()<<"Closed the factory."<<retval;
            hFactory = NULL;
        }
    }
    FACTORY_HANDLE hFactory;
};

static GigeSingleOnePrivate s_gigeFactory;

class GigeCameraBasePrivate : public CJDummyClass
{
    friend class JaiGigeCamera;
public:
    GigeCameraBasePrivate()
        : hThread(NULL)
        , hCamera(NULL)
        , streamCount(0)
        , width(0)
        , height(0)
        , frameCount(0)
        , hGainNode(NULL)
        , hExposureNode(NULL)
        , minGain(0)
        , maxGain(0)
        , minExposure(0)
        , maxExposure(0)
        , currentExposure(0)
        , useHistogram(false)
        , playStreamNum(-1)
        , m_bThreadRunning(false)
        , m_hEventKill(NULL)
        , m_hConnectionSupervisionThread(NULL)
        , m_bEnableThread(false)

     {
        memset(&YBufferInfo, 0, sizeof(YBufferInfo));
        memset(&BufferInfo, 0, sizeof(BufferInfo));
        memset(&histogramInfo, 0, sizeof(histogramInfo));
    }
    bool openCamera(QString cameraID);
    void CloseCamera();
    bool startPlayStream(int streamNum, bool useHistogram);
    bool configCamera();
    bool StopPlayStream();
    void gigeDecodeCallback(J_tIMAGE_INFO * pAqImageInfo);
public:
    THRD_HANDLE hThread;
    NODE_HANDLE hGainNode;     // Handle to "GainRaw" node
    NODE_HANDLE hExposureNode; // Handle to "ExposureTimeRaw" node
    uint32_t    streamCount;
    int         width;
    int         height;
    int         frameCount;
    int64_t     minGain;
    int64_t     maxGain;
    double      minExposure;
    double      maxExposure;
    double      currentExposure;
    bool        useHistogram;
public:
    QElapsedTimer   t;
    QElapsedTimer   frameValid;
    JaiGSFrame      frame;
    QMutex          lock;
    J_tHIST         histogramInfo;
    QList<int>      redHist;
    QList<int>      greenHist;
    QList<int>      blueHist;
    J_tIMAGE_INFO   YBufferInfo;
    J_tIMAGE_INFO   BufferInfo;
private:
    void ConnectionSupervisionProcess();
    static  DWORD ProcessCaller(GigeCameraBasePrivate *pThread);
private:
    CAM_HANDLE      hCamera;
    int             playStreamNum;
    bool            m_bThreadRunning;
    HANDLE	        m_hEventKill;      // Event used for speeding up the termination of the supervision thread
    HANDLE	        m_hConnectionSupervisionThread;
    bool            m_bEnableThread;
};

void GigeCameraBasePrivate::gigeDecodeCallback(J_tIMAGE_INFO * pAqImageInfo)
{
    J_STATUS_TYPE retval = J_ST_SUCCESS;
    // Shows image
//        qDebug()<<"=============="<<pAqImageInfo->iBlockId;
//        qDebug()<<"iAnnouncedBuffers"<<pAqImageInfo->iAnnouncedBuffers;
//        qDebug()<<"iAwaitDelivery"<<pAqImageInfo->iAwaitDelivery;
//        qDebug()<<"iBlockId"<<pAqImageInfo->iBlockId;
//        qDebug()<<"iImageSize"<<pAqImageInfo->iImageSize;
//        qDebug()<<"iMissingPackets"<<pAqImageInfo->iMissingPackets;
//        qDebug()<<"iOffsetX"<<pAqImageInfo->iOffsetX;
//        qDebug()<<"iOffsetY"<<pAqImageInfo->iOffsetY;
//        qDebug()<<"iPixelType"<<pAqImageInfo->iPixelType;
//        qDebug()<<"iQueuedBuffers"<<pAqImageInfo->iQueuedBuffers;
//        qDebug()<<"iSizeX"<<pAqImageInfo->iSizeX;
//        qDebug()<<"iSizeY"<<pAqImageInfo->iSizeY;
//        qDebug()<<"iTimeStamp"<<pAqImageInfo->iTimeStamp;
//        qDebug()<<"pImageBuffer"<<pAqImageInfo->pImageBuffer;
//        J_Image_ShowImage(g_hView, pAqImageInfo);
    frameCount++;
    qreal ms = t.elapsed();
    static qreal fps = 0;
    if (ms > 1000) {
        t.restart();
        fps = frameCount * 1000 / ms;
        qWarning()<<"fps: "<<fps;
        frameCount = 0;
    }
    if (YBufferInfo.pImageBuffer == NULL) {
        retval = J_Image_Malloc(pAqImageInfo, &YBufferInfo);
        if (J_ST_SUCCESS != retval) {
            qWarning()<<QString("Error with J_Image_Malloc in gigeDecodeCallback. %1").arg(retval);
            return;
        }
    }

    if (useHistogram) {
        if (histogramInfo.iColors == 0) {
            retval = J_Image_MallocHistogram (pAqImageInfo, &histogramInfo);
            if (retval != J_ST_SUCCESS) {
                qWarning()<<QString("Error with J_Image_MallocHistogram in gigeDecodeCallback. %1").arg(retval);
                return;
            }
        }
        retval = J_Image_ClearHistogram(&histogramInfo);
        if (retval != J_ST_SUCCESS) {
            qWarning()<<QString("Error with J_Image_ClearHistogram in gigeDecodeCallback. %1").arg(retval);
            return;
        }
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        retval = J_Image_CreateHistogram(pAqImageInfo, &rect, &histogramInfo);
        if (retval != J_ST_SUCCESS) {
            qWarning()<<QString("Error with J_Image_CreateHistogram in gigeDecodeCallback. %1").arg(retval);
            return;
        }
        int count = histogramInfo.iHistEntries;
        QList<int>* ptrTable[] = {&redHist, &greenHist, &blueHist};
        for (int j = 0; j < (int)histogramInfo.iColors; j++) {
            uint32_t  val = 0;
            if (ptrTable[j]->count() == count) {
                for (int i = 0; i < count; i++) {
                    J_Image_GetHistogramValue(&histogramInfo, j, i, &val);
                    (*ptrTable[j])[i] = (int)val;
                }
            } else {
                ptrTable[j]->clear();
                for (int i = 0; i < count; i++) {
                    J_Image_GetHistogramValue(&histogramInfo, j, i, &val);
                    ptrTable[j]->append(val);
                }
            }
        }
    }
    lock.lock();
    if (pAqImageInfo->iPixelType == J_GVSP_PIX_MONO8) {
        if (J_ST_SUCCESS == J_Image_FromRawToImage(pAqImageInfo, &YBufferInfo))  {
            frame.nType = JaiFrameGray8Bit;
            frame.nWidth = YBufferInfo.iSizeX;
            frame.nHeight = YBufferInfo.iSizeY;
            frame.nStamp = YBufferInfo.iTimeStamp;
            frame.dwFrameNum = YBufferInfo.iBlockId;
            frame.data = QByteArray((const char *)YBufferInfo.pImageBuffer, YBufferInfo.iImageSize);
            frameValid.restart();
        }
    } else if (pAqImageInfo->iPixelType == J_GVSP_PIX_MONO10) {
        if (J_ST_SUCCESS == J_Image_FromRawToImage(pAqImageInfo, &YBufferInfo))  {
            frame.nType = JaiFrameGray10BitOCCU2ByteMSB;
            frame.nWidth = YBufferInfo.iSizeX;
            frame.nHeight = YBufferInfo.iSizeY;
            frame.nStamp = YBufferInfo.iTimeStamp;
            frame.dwFrameNum = YBufferInfo.iBlockId;
            frame.data = QByteArray((const char *)YBufferInfo.pImageBuffer, YBufferInfo.iImageSize);
            frameValid.restart();
        }
    } else if (pAqImageInfo->iPixelType == J_GVSP_PIX_MONO12) {
        if (J_ST_SUCCESS == J_Image_FromRawToImage(pAqImageInfo, &YBufferInfo))  {
            frame.nType = JaiFrameGray12BitOCCU2ByteMSB;
            frame.nWidth = YBufferInfo.iSizeX;
            frame.nHeight = YBufferInfo.iSizeY;
            frame.nStamp = YBufferInfo.iTimeStamp;
            frame.dwFrameNum = YBufferInfo.iBlockId;
            frame.data = QByteArray((const char *)YBufferInfo.pImageBuffer, YBufferInfo.iImageSize);
            frameValid.restart();
        }
    } else if ((pAqImageInfo->iPixelType & J_GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK) == J_GVSP_PIX_OCCUPY24BIT) {
        if (BufferInfo.pImageBuffer == NULL) {
            retval = J_Image_MallocEx(pAqImageInfo, &BufferInfo, PF_24BIT_SWAP_RB);
            if (J_ST_SUCCESS != retval) {
                qWarning()<<QString("Error with J_Image_Malloc in gigeDecodeCallback. %1").arg(retval);
                lock.unlock();
                return;
            }
        }
        if (J_ST_SUCCESS == J_Image_FromRawToImageEx(pAqImageInfo, &YBufferInfo, BAYER_EXTEND))  {
            J_STATUS_TYPE status = J_Image_ConvertImage(&YBufferInfo, &BufferInfo, PF_24BIT_SWAP_RB);
            if (J_ST_SUCCESS == status) {
                frame.nType = JaiFrameRGB888;
                frame.nWidth = BufferInfo.iSizeX;
                frame.nHeight = BufferInfo.iSizeY;
                frame.nStamp = BufferInfo.iTimeStamp;
                frame.dwFrameNum = BufferInfo.iBlockId;
                frame.data = QByteArray((const char *)BufferInfo.pImageBuffer, BufferInfo.iImageSize);
                frameValid.restart();
            } else {
                qDebug()<<"error: "<<status;
            }
        }
    }
    lock.unlock();
}

DWORD GigeCameraBasePrivate::ProcessCaller(GigeCameraBasePrivate* pThread)
{
    pThread->ConnectionSupervisionProcess();
    return 0;
}

//--------------------------------------------------------------------------------------------------
// OpenFactoryAndCamera
//--------------------------------------------------------------------------------------------------
bool GigeCameraBasePrivate::openCamera(QString cameraID)
{
    int64_t int64Val;
    J_STATUS_TYPE retval;

    retval = J_Camera_Open(s_gigeFactory.hFactory, (int8_t*)cameraID.toLatin1().data(), &hCamera);
    if (retval != J_ST_SUCCESS) {
        qDebug()<<"Could not open the camera!"<<retval<<cameraID;
        return false;
    }

    //This sample is appropriate only for GigE cameras.
    int8_t buffer[J_CAMERA_ID_SIZE] = {0};
    uint32_t mySize = J_CAMERA_ID_SIZE;
    retval = J_Camera_GetTransportLayerName(hCamera, buffer, &mySize);
    if (retval != J_ST_SUCCESS) {
        qWarning("Error calling J_Camera_GetTransportLayerName.");
        return false;
    }

    //Convert to upper case
    std::string strTransportName((char*)buffer);
    std::transform(strTransportName.begin(), strTransportName.end(), strTransportName.begin(), (int( *)(int)) toupper);

    if(std::string::npos == strTransportName.find("GEV") && std::string::npos == strTransportName.find("GIGEVISION")) {
        qDebug()<<"This sample only works with GigE cameras, but cannot find the GigE Driver!";
        return false;
    }

    // Get Width from the camera based on GenICam name
    retval = J_Camera_GetValueInt64(hCamera, NODE_NAME_WIDTH, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<QString("Could not get Width value! ret:")<<retval;
    }
    width = (int)int64Val;

    // Get Height from the camera
    retval = J_Camera_GetValueInt64(hCamera, NODE_NAME_HEIGHT, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<QString("Could not get Height value! ret:")<<retval;
    }
    height = (int)int64Val;

    // Get the pixelformat from the camera
    int64_t pixelFormat = 0;
    uint64_t jaiPixelFormat = 0;
    retval = J_Camera_GetValueInt64(hCamera, NODE_NAME_PIXELFORMAT, &pixelFormat);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"Unable to get PixelFormat value!"<<retval;
    } else {
        J_Image_Get_PixelFormat(hCamera, pixelFormat, &jaiPixelFormat);
    }
    // Calculate number of bits (not bytes) per pixel using macro
    int bpp = J_BitsPerPixel(jaiPixelFormat);
//    uint32_t streamCount = 0;
    J_Camera_GetNumOfDataStreams(hCamera, &streamCount);
//    streamCount = streamCount;
//    this->cameraID = cameraID;
    qDebug()<<QString("open camera %1 success, width %2 height: %3, stramCount: %4, pixelFormat: %5 pixelBpp: %6")
              .arg(cameraID).arg(width).arg(height).arg(streamCount).arg(jaiPixelFormat).arg(bpp);
    return true;
}

bool GigeCameraBasePrivate::startPlayStream(int streamNum, bool useHistogram)
{
    // Is it already created?
    if (m_hConnectionSupervisionThread) {
        return false;
    }

    // Set the thread execution flag
    m_bEnableThread = true;
    m_bThreadRunning = false;

    playStreamNum = streamNum;
    // Create a Stream Thread.
    if (NULL == (m_hConnectionSupervisionThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ProcessCaller,
                                                              this, NULL, NULL))) {
        qDebug("*** Error : CreateThread\n");
        return false;
    }

    t.start();
    useHistogram = useHistogram;

    return true;
}

bool GigeCameraBasePrivate::configCamera()
{
    J_STATUS_TYPE retVal;
    DEV_HANDLE hParent = 0;
    if (hCamera == NULL) {
        qWarning()<<"must open camera before configCamera.";
        return false;
    }
    retVal = J_Camera_GetLocalDeviceHandle(hCamera, &hParent);
    if (J_ST_SUCCESS != retVal) {
        qWarning()<<"Error getting local device handle."<<retVal;
        return false;
    }

    //Enable
    int64_t passCorrupt = 1;
    retVal = J_Camera_SetValueInt64(hParent, (int8_t*)"PassCorruptFrames", passCorrupt);
    if(J_ST_SUCCESS != retVal) {
        qWarning()<<"Error setting PassCorruptFrames."<<retVal;
    }

    int64_t enablePacketResend = 1;
    retVal = J_Camera_SetValueInt64(hParent, (int8_t*)"EnablePacketResend", enablePacketResend);
    if(J_ST_SUCCESS != retVal) {
        qWarning()<<"Error setting EnablePacketResend."<<retVal;
    }
    return true;
}

void GigeCameraBasePrivate::CloseCamera()
{
    if (hCamera) {
        // Close camera
        J_Camera_Close(hCamera);
        hCamera = NULL;
        qDebug("Closed camera\n");
    }
}

//==============================================================////
// Terminate Connection Event Thread
//==============================================================////
bool GigeCameraBasePrivate::StopPlayStream(void)
{
    // Reset the thread execution flag.
    m_bEnableThread = false;

    // Signal the thread to stop faster
    SetEvent(m_hEventKill);

    // Wait for the thread to end
    while (m_bThreadRunning) {
        Sleep(100);
    }

    if(m_hConnectionSupervisionThread) {
        CloseHandle(m_hConnectionSupervisionThread);
        m_hConnectionSupervisionThread = NULL;
    }

    t.invalidate();

    if (YBufferInfo.pImageBuffer) {
        J_Image_Free(&YBufferInfo);
        YBufferInfo.pImageBuffer = NULL;
    }
    if (BufferInfo.pImageBuffer) {
        J_Image_Free(&BufferInfo);
        BufferInfo.pImageBuffer = NULL;
    }
    if (histogramInfo.iColors != 0) {
         J_Image_FreeHistogram(&histogramInfo);
         memset(&histogramInfo, 0, sizeof(histogramInfo));
    }
    return true;
}

//==============================================================////
// Stream Processing Function
//==============================================================////
void GigeCameraBasePrivate::ConnectionSupervisionProcess(void)
{
    uint32_t    iSize;
    J_EVENT_DATA_CONNECTION	  DataConnectionInfo;
    J_STATUS_TYPE retval;

    // Mark thread as running
    m_bThreadRunning = true;

    DWORD	iWaitResult;
    HANDLE	hEventConnectionChanged = CreateEvent(NULL, false, false, NULL);

    // Is the kill event already defined?
    if (m_hEventKill != NULL) {
        CloseHandle(m_hEventKill);
        m_hEventKill = NULL;
    }

    // Create a new one
    m_hEventKill = CreateEvent(NULL, true, false, NULL);

    // Make a list of events to be used in WaitForMultipleObjects
    HANDLE	EventList[2];
    EventList[0] = hEventConnectionChanged;
    EventList[1] = m_hEventKill;

    EVT_HANDLE hEvent = NULL; // Connection event handle

    // Register the connection event with the transport layer
    retval = J_Camera_RegisterEvent(hCamera, EVENT_CONNECTION, hEventConnectionChanged, &hEvent);

    if(J_ST_SUCCESS == retval) {
        qDebug(">>> Start Connection Supervision Process Loop.");

        while (m_bEnableThread) {
            // Wait for Connection event (or kill event)
            iWaitResult = WaitForMultipleObjects(2, EventList, false, 1000);

            // Did we get a connection change event?
            if (WAIT_OBJECT_0 == iWaitResult) {
                // Get the Connection Info from the event
                iSize = (uint32_t)sizeof(J_EVENT_DATA_CONNECTION);
                retval = J_Event_GetData(hEvent, &DataConnectionInfo,  &iSize);

                if (retval == J_ST_SUCCESS) {
                    // Update the status label
                    switch(DataConnectionInfo.m_iConnectionState) {
                    case J_EDC_CONNECTED:
                        qDebug("Camera just connected");
                        // If the Live Video display has not been started yet then start it using the J_Image_OpenStreamLight()
                        if (hThread == NULL) {
                            qDebug()<<"begin to open camera.";
                            uint32_t buffSize = width * height * 6;
                            // Open stream
                            retval = J_Image_OpenStream(hCamera, (uint32_t)playStreamNum, this
                                                        , reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&GigeCameraBasePrivate::gigeDecodeCallback)
                                                        , &hThread, buffSize);
                            if (retval != J_ST_SUCCESS) {
                                qWarning()<<"J_Image_OpenStream failed."<<retval;
                            }
                            // Acquisition Start
                            if (J_Camera_ExecuteCommand(hCamera, NODE_NAME_ACQSTART) != J_ST_SUCCESS) {
                                qWarning()<<"AcquisitionStart failed.";
                            }
                        } else {
                            qDebug()<<"no camera thread.";
                        }
                        break;
                    case J_EDC_DISCONNECTED:
                    case J_EDC_LOST_CONTROL:
                        qDebug(" Camera just disconnected");

                        // We lost connection so we want to close the stream channel so it can be restarted later on
                        // when the camera is reconnected
                        if (hThread) {
                            // Close stream
                            retval = J_Image_CloseStream(hThread);
                            if (retval != J_ST_SUCCESS) {
                                qDebug("Could not close Stream!");
                            }
                            hThread = NULL;
                            qDebug("Closed stream\n");
                        }
                        break;
//                    case J_EDC_LOST_CONTROL:
//                        qDebug(" We lost control of the camera");
//                        break;
                    case J_EDC_GOT_CONTROL: // Deprecated!!
                        qDebug("We regained control of the camera");
                        break;
                    }
                }
            } else {
                switch(iWaitResult) {
                    // Kill event
                case	WAIT_OBJECT_0 + 1:
                    break;
                    // Timeout
                case	WAIT_TIMEOUT:
                    break;
                    // Unknown?
                default:
                    break;
                }
            }

            // Here we need to see if more events has been queued up!
            uint32_t    NumEventsInQueue = 0;
            iSize = sizeof(NumEventsInQueue);

            retval = J_Event_GetInfo(hEvent, (J_EVENT_INFO_ID)EVENT_INFO_NUM_ENTRYS_IN_QUEUE, &NumEventsInQueue, &iSize);

            if ((retval == J_ST_SUCCESS) && (NumEventsInQueue > 0)) {
                SetEvent(hEventConnectionChanged);
            }

        }//while(m_bEnableThread)

    }//if(J_ST_SUCCESS == retval)

    qDebug(" Terminated Connection Supervision Process Loop.");

    J_Camera_UnRegisterEvent(hCamera, EVENT_CONNECTION);
    // Free the event object
    if (hEvent != NULL) {
        J_Event_Close(hEvent);
        hEvent = NULL;
    }

    // Free the kill event
    if (m_hEventKill != NULL) {
        CloseHandle(m_hEventKill);
        m_hEventKill = NULL;
    }

    // Free the connection event object
    if (hEventConnectionChanged != NULL) {
        CloseHandle(hEventConnectionChanged);
        hEventConnectionChanged = NULL;
    }

    m_bThreadRunning = false;
}

JaiGigeCamera::JaiGigeCamera()
    : p_d(new GigeCameraBasePrivate)
{
}

JaiGigeCamera::~JaiGigeCamera()
{
    closeCamera();
    delete p_d;
}

bool JaiGigeCamera::closeCamera()
{
    J_STATUS_TYPE retval = J_ST_SUCCESS;
    // Close camera
    stopPlayStream();
    p_d->CloseCamera();

    p_d->hGainNode = NULL;
    p_d->streamCount = 0;
    p_d->width = 0;
    p_d->height = 0;
    p_d->frameCount = 0;
    p_d->minGain = 0;
    p_d->maxGain = 0;
    p_d->minExposure = 0;
    p_d->maxExposure = 0;
    p_d->useHistogram = false;
    p_d->hExposureNode = NULL;

    return retval == J_ST_SUCCESS;
}

QStringList JaiGigeCamera::getDeviceList(QStringList& outIdList)
{
    return s_gigeFactory.getDeviceList(outIdList);
}

bool JaiGigeCamera::openCamera(QString cameraID)
{
    closeCamera();
    return p_d->openCamera(cameraID);
}

bool JaiGigeCamera::configCamera()
{
    return p_d->configCamera();
}

bool JaiGigeCamera::startPlayStream(int streamNum, bool useHistogram)
{   
    return p_d->startPlayStream(streamNum, useHistogram);
}

bool JaiGigeCamera::stopPlayStream()
{
    return p_d->StopPlayStream();
}

bool JaiGigeCamera::getFrame(JaiGSFrame &frame, QList<int> *redHis, QList<int> *greenHis, QList<int> *blueHis)
{
    p_d->lock.lock();
    if ((p_d->frameValid.elapsed() > 5000) || p_d->frame.data.isEmpty()) {
        p_d->lock.unlock();
        return false;
    }
    frame = p_d->frame;
    p_d->frame.data.detach();
    p_d->lock.unlock();

    if (p_d->useHistogram) {
        if (redHis) {
            *redHis = p_d->redHist;
        }
        if (greenHis) {
            *greenHis = p_d->greenHist;
        }
        if (blueHis) {
            *blueHis = p_d->blueHist;
        }
    }
    return true;
}

//Utility function to set the frame grabber's width/height (if one is present in the system).
bool JaiGigeCamera::setFramegrabberValue(const QString name, int64_t int64Val)
{
    //Set frame grabber value, if applicable
    DEV_HANDLE hDev = NULL; //If a frame grabber exists, it is at the GenTL "local device layer".
    J_STATUS_TYPE retval = J_Camera_GetLocalDeviceHandle(p_d->hCamera, &hDev);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get device handle."<<retval;
        return false;
    }

    if (NULL == hDev) {
        qWarning()<<"cannot get device handle, handle is null"<<retval;
        return false;
    }

    NODE_HANDLE hNode;
    retval = J_Camera_GetNodeByName(hDev, (int8_t*)name.toLatin1().data(), &hNode);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get node name "<<retval;
        return false;
    }

    retval = J_Node_SetValueInt64(hNode, false, int64Val);
    if (J_ST_SUCCESS != retval) {
        return false;
    }

    //Special handling for Active Silicon CXP boards, which also has nodes prefixed
    //with "Incoming":
    if (cameraID.contains("TLActiveSilicon")) {
        if (name.contains("Width") || name.contains("Height")) {
            QString strIncoming = "Incoming" + name;
            NODE_HANDLE hNodeIncoming;
            J_STATUS_TYPE retval = J_Camera_GetNodeByName(hDev, (int8_t*)strIncoming.toLatin1().data(), &hNodeIncoming);
            if (retval == J_ST_SUCCESS) {
                retval = J_Node_SetValueInt64(hNodeIncoming, false, int64Val);
            } else {
                qWarning()<<"cannot set node value"<<strIncoming;
                return false;
            }
        }
    }
    return true;
}

//Utility function to set the frame grabber's pixel format (if one is present in the system).
bool JaiGigeCamera::SetFramegrabberPixelFormat(QString name, int64_t jaiPixelFormat)
{
    int8_t szJaiPixelFormatName[512];
    uint32_t iSize = 512;
    J_STATUS_TYPE retval = J_Image_Get_PixelFormatName(p_d->hCamera, jaiPixelFormat, szJaiPixelFormatName, iSize);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get pixelFormat, ret"<<jaiPixelFormat<<retval;
        return false;
    }

    NODE_HANDLE hLocalDeviceNode = 0;
    retval = J_Camera_GetNodeByName(p_d->hCamera, (int8_t *)"PixelFormat", &hLocalDeviceNode);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get node by name, ret"<<p_d->hCamera<<retval;
        return false;
    }

    if (0 == hLocalDeviceNode) {
        qWarning()<<"cannot get pixelFormat, mybe no pixelFormat node. ret"<<retval;
        return false;
    }

    //NOTE: this may fail if the camera and/or frame grabber does not use the SFNC naming convention for pixel formats!
    //Check the camera and frame grabber for details.
    retval = J_Node_SetValueString(hLocalDeviceNode, false, szJaiPixelFormatName);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot set pixelFormat"<<szJaiPixelFormatName<<" ret"<<retval;
        return false;
    }

    //Special handling for Active Silicon CXP boards, which also has nodes prefixed
    //with "Incoming":
    if (cameraID.contains("TLActiveSilicon")) {
        QString strIncoming = "Incoming" + name;
        NODE_HANDLE hNodeIncoming;
        J_STATUS_TYPE retval = J_Camera_GetNodeByName(p_d->hCamera, (int8_t*)strIncoming.toLatin1().data(), &hNodeIncoming);
        if (retval == J_ST_SUCCESS) {
            //NOTE: this may fail if the camera and/or frame grabber does not use the SFNC naming convention for pixel formats!
            //Check the camera and frame grabber for details.
            retval = J_Node_SetValueString(hNodeIncoming, false, szJaiPixelFormatName);
        } else {
            qWarning()<<"cannot set node value"<<strIncoming;
            return false;
        }
    }
    return true;
}

void JaiGigeCamera::dumpCameraCategoryNode()
{
    J_STATUS_TYPE retval;
    uint32_t        nFeatureNodes;
    NODE_HANDLE     hNode;
    int8_t          sNodeName[256], sSubNodeName[256];
    uint32_t        size;

    if (p_d->hCamera == NULL) {
        qWarning()<<"must open camera before dumpNode.";
        return;
    }
    retval = J_Camera_GetNumOfSubFeatures(p_d->hCamera, (int8_t*)&J_ROOT_NODE, &nFeatureNodes);
    if (retval == J_ST_SUCCESS){
        qDebug("===== %u feature nodes were found in the Root node for camera =====\n", nFeatureNodes, cameraID);
        // Run through the list of feature nodes and print out the names
        for (uint32_t index = 0; index < nFeatureNodes; ++index) {    // Get subfeature node handle
            retval = J_Camera_GetSubFeatureByIndex(p_d->hCamera, (int8_t*)&J_ROOT_NODE, index, &hNode);
            if (retval == J_ST_SUCCESS) {      // Get subfeature node name
                size = sizeof(sNodeName);
                retval = J_Node_GetName(hNode, sNodeName, &size, 0);
                if (retval == J_ST_SUCCESS) {        // Print out the name
                    qDebug("%u: %s", index, sNodeName);
                }      // Get the node type
                J_NODE_TYPE NodeType;
                retval = J_Node_GetType(hNode, &NodeType);    // Is this a category node?
                if (NodeType == J_ICategory) {           // Get number of sub features under the category node
                    uint32_t nSubFeatureNodes;
                    retval = J_Camera_GetNumOfSubFeatures(p_d->hCamera, sNodeName, &nSubFeatureNodes);
                    if (nSubFeatureNodes > 0)        {
                        qDebug("\t%u subfeature nodes were found", nSubFeatureNodes);
                        for (uint32_t subindex = 0; subindex < nSubFeatureNodes; subindex++)          {
                            NODE_HANDLE hSubNode;
                            retval = J_Camera_GetSubFeatureByIndex(p_d->hCamera, sNodeName, subindex, &hSubNode);
                            if (retval == J_ST_SUCCESS)            {
                                size = sizeof(sSubNodeName);
                                retval = J_Node_GetName(hSubNode, sSubNodeName, &size, 0); // Print out the name
                                qDebug("\t%u-%u: %s", index, subindex, sSubNodeName);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool JaiGigeCamera::setSize(int width, int height)
{
    J_STATUS_TYPE   retval;
    NODE_HANDLE     hNode;
    int64_t int64Val;

    // Get Width Node
    retval = J_Camera_GetNodeByName(p_d->hCamera, NODE_NAME_WIDTH, &hNode);
    if (retval == J_ST_SUCCESS) {
        // Get/Set Min
        retval = J_Node_GetMinInt64(hNode, &int64Val);
        if (width < int64Val) {
            qWarning()<<"node value with < nodeMinVal"<<int64Val;
            return false;
        }
        // Get/Set Max
        retval = J_Node_GetMaxInt64(hNode, &int64Val);
        if (width > int64Val) {
            qWarning()<<"node value with > nodeMaxVal"<<int64Val;
            return false;
        }
        int64Val = width;
        //Set frame grabber dimension, if applicable
        if (!setFramegrabberValue("Width", int64Val)) {
            return false;
        }
    }

    //- Height ----------------------------------------------
    retval = J_Camera_GetNodeByName(p_d->hCamera, NODE_NAME_HEIGHT, &hNode);
    if (retval == J_ST_SUCCESS) {
        // Get/Set Min
        retval = J_Node_GetMinInt64(hNode, &int64Val);
        if (height < int64Val) {
            qWarning()<<"node value height < nodeMinVal"<<int64Val;
            return false;
        }
        // Get/Set Max
        retval = J_Node_GetMaxInt64(hNode, &int64Val);
        if (height > int64Val) {
            qWarning()<<"node value height > nodeMaxVal"<<int64Val;
            return false;
        }
        int64Val = height;
        //Set frame grabber dimension, if applicable
        if (setFramegrabberValue("height", int64Val)) {
            return false;
        }
    }
    return true;
}

bool JaiGigeCamera::setPixelFormat(QString pixelFormat)
{
    int64_t type;
    if (pixelFormat == "Mono8") {
        type = 17301505;
    } else if (pixelFormat == "Mono10") {
        type = 17825795;
    } else if (pixelFormat == "Mono12") {
        type = 17825797;
    } else if (pixelFormat == "RGB8Packed") {
        type = 35127316;
    } else {
        return false;
    }
    return SetFramegrabberPixelFormat(pixelFormat, type);
}

bool JaiGigeCamera::getGain(int& minGain, int& maxGain, int& currentGain)
{
    J_STATUS_TYPE   retval;
    int64_t         int64Val;
    if (p_d->hGainNode == NULL) {
        retval = J_Camera_GetNodeByName(p_d->hCamera, NODE_NAME_GAIN, &p_d->hGainNode);
        if (retval != J_ST_SUCCESS) {
            p_d->hGainNode = NULL;
            qWarning()<<"cannot get node handel. "<<NODE_NAME_GAIN<<retval;
            return false;
        }
        // Get/Set Min
        retval = J_Node_GetMinInt64(p_d->hGainNode, &int64Val);
        p_d->minGain = int64Val;
        // Get/Set Max
        retval = J_Node_GetMaxInt64(p_d->hGainNode, &int64Val);
        p_d->maxGain = int64Val;
    }
    minGain = (int)p_d->minGain;
    maxGain = (int)p_d->maxGain;

    // Get/Set Value
    retval = J_Node_GetValueInt64(p_d->hGainNode, TRUE, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"cannot get current gain. ret"<<retval;
        return false;
    }
    currentGain = (int)int64Val;
    return true;
}

bool JaiGigeCamera::setGain(int gain)
{
    J_STATUS_TYPE   retval;

    int minGain, maxGain, currentGain;
    if (!getGain(minGain, maxGain, currentGain)) {
         return false;
    }

    if ((gain > maxGain) || (gain < minGain)) {
        qWarning()<<QString("the gain out of the range (%1,%2)").arg(minGain).arg(maxGain);
        return false;
    }
    retval = J_Node_SetValueInt64(p_d->hGainNode, TRUE, (int64_t)gain);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"set gain node value "<<gain<<" failed. "<<retval;
        return false;
    }
    return true;
}

bool JaiGigeCamera::getExposureTime(double& minExposure, double& maxExposure, double& exposure)
{
    J_STATUS_TYPE   retval;
    double          double64Val;

    if (p_d->hExposureNode == NULL) {
        retval = J_Camera_GetNodeByName(p_d->hCamera, NODE_NAME_EXPOSURE, &p_d->hExposureNode);
        if (retval != J_ST_SUCCESS) {
            p_d->hExposureNode = NULL;
            qWarning()<<"cannot get node handel. "<<NODE_NAME_EXPOSURE<<retval;
            return false;
        }
        // Get/Set Min
        retval = J_Node_GetMinDouble(p_d->hExposureNode, &double64Val);
        p_d->minExposure = double64Val;
        // Get/Set Max
        retval = J_Node_GetMaxDouble(p_d->hExposureNode, &double64Val);
        p_d->maxExposure = double64Val;
        if (retval != J_ST_SUCCESS) {
            p_d->hExposureNode = NULL;
            qWarning()<<"cannot get max min Exposure handle. ret"<<retval<<p_d->hExposureNode;
            return false;
        }
    }

    minExposure = (int)p_d->minExposure;
    maxExposure = (int)p_d->maxExposure;

    // Get/Set Value
    retval = J_Node_GetValueDouble(p_d->hExposureNode, TRUE, &double64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"cannot get current Exposure. ret"<<retval<<p_d->hExposureNode;
        return false;
    }
    exposure = double64Val;
    return true;
}

bool JaiGigeCamera::setExposureTime(double exposure)
{
    J_STATUS_TYPE   retval;
    // We have two possible ways of setting up Exposure time: JAI or GenICam SFNC
    // The JAI Exposure time setup uses a node called "ShutterMode" and the SFNC
    // does not need to set up anything in order to be able to control the exposure time.
    // Therefor we have to determine which way to use here.
    // First we see if a node called "ShutterMode" exists.
    if (fabs(exposure - p_d->currentExposure) < 0.1) {
        return true;
    }
    NODE_HANDLE hShtterNode = NULL;
    retval = J_Camera_GetNodeByName(p_d->hCamera, (int8_t*)"ShutterMode", &hShtterNode);
    // Does the "ShutterMode" node exist?
    if ((retval == J_ST_SUCCESS) && (hShtterNode != NULL)) {
        // Here we assume that this is JAI way so we do the following:
        // ShutterMode=ProgrammableExposure
        // Make sure that the ShutterMode selector is set to ProgrammableExposure
        retval = J_Camera_SetValueString(p_d->hCamera, (int8_t*)"ShutterMode", (int8_t*)"ProgrammableExposure");
        if (retval != J_ST_SUCCESS) {
            qWarning()<<"cannot set camera shurrermode for jai camera."<<retval;
        }
    }

    double minExposure, maxExposure;
    if (!getExposureTime(minExposure, maxExposure, p_d->currentExposure)) {
         return false;
    }

    if ((exposure > maxExposure) || (exposure < minExposure)) {
        qWarning()<<QString("the exposure out of the range (%1,%2)").arg(minExposure).arg(maxExposure);
        return false;
    }

    retval = J_Node_SetValueDouble(p_d->hExposureNode, TRUE, exposure);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"set exposure node value "<<exposure<<" failed. "<<retval;
        return false;
    }
    p_d->currentExposure = exposure;
    return true;
}
