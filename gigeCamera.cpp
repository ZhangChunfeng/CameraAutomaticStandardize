#include <QDebug>
#include <QElapsedTimer>
#include "gigeCamera.h"
#include <qt_windows.h>
#include "jai_factory.h"
#include "Jai_Error.h"

#define NODE_NAME_WIDTH         (int8_t*)"Width"
#define NODE_NAME_HEIGHT        (int8_t*)"Height"
#define NODE_NAME_PIXELFORMAT   (int8_t*)"PixelFormat"
#define NODE_NAME_GAIN          (int8_t*)"GainRaw"
#define NODE_NAME_EXPOSURE      (int8_t*)"ExposureTimeRaw"
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
            retval = J_Factory_Close(hFactory);
            qDebug()<<"Closed the factory."<<retval;
            hFactory = NULL;
        }
    }
    FACTORY_HANDLE hFactory;
};

static GigeSingleOnePrivate s_gigeFactory;

class GigeCameraBasePrivate : public CJDummyClass
{
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
        , useHistogram(false)
     {
        memset(&YBufferInfo, 0, sizeof(YBufferInfo));
        memset(&BufferInfo, 0, sizeof(BufferInfo));
        memset(&histogramInfo, 0, sizeof(histogramInfo));
    }
    void gigeDecodeCallback(J_tIMAGE_INFO * pAqImageInfo);
public:
    THRD_HANDLE hThread;
    CAM_HANDLE  hCamera;
    NODE_HANDLE hGainNode;     // Handle to "GainRaw" node
    NODE_HANDLE hExposureNode; // Handle to "ExposureTimeRaw" node
    int         streamCount;
    int         width;
    int         height;
    int         frameCount;
    int64_t     minGain;
    int64_t     maxGain;
    int64_t     minExposure;
    int64_t     maxExposure;
    bool        useHistogram;
public:
    QElapsedTimer   t;
    QElapsedTimer   frameValid;
    JaiGSFrame         frame;
    J_tHIST         histogramInfo;
    QList<int>      redHist;
    QList<int>      greenHist;
    QList<int>      blueHist;
    J_tIMAGE_INFO   YBufferInfo;
    J_tIMAGE_INFO   BufferInfo;
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

    if ((pAqImageInfo->iPixelType & J_GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK) == J_GVSP_PIX_OCCUPY10BIT) {
        if (J_ST_SUCCESS == J_Image_FromRawToImage(pAqImageInfo, &YBufferInfo))  {
            frame.nType = JaiFrameGray;
            frame.nWidth = YBufferInfo.iSizeX;
            frame.nHeight = YBufferInfo.iSizeY;
            frame.nStamp = YBufferInfo.iTimeStamp;
            frame.dwFrameNum = YBufferInfo.iBlockId;
            frame.data = QByteArray((const char *)YBufferInfo.pImageBuffer, YBufferInfo.iImageSize);
            frameValid.restart();
        }
    } else {
        if (BufferInfo.pImageBuffer == NULL) {
            retval = J_Image_MallocEx(pAqImageInfo, &BufferInfo, PF_24BIT_SWAP_RB);
            if (J_ST_SUCCESS != retval) {
                qWarning()<<QString("Error with J_Image_Malloc in gigeDecodeCallback. %1").arg(retval);
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
}

GigeCamera::GigeCamera()
    : p_d(new GigeCameraBasePrivate)
{
}

GigeCamera::~GigeCamera()
{
    closeCamera();
    delete p_d;
}

bool GigeCamera::closeCamera()
{
    J_STATUS_TYPE retval = J_ST_SUCCESS;
    // Close camera
    stopPlayStream();
    if (p_d->hCamera) {
        retval = J_Camera_Close(p_d->hCamera);
        qDebug()<<QString("Close the camera %1. ret").arg(cameraID)<<retval;
        p_d->hCamera = NULL;
    }

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

QStringList GigeCamera::getDeviceList(QStringList& outIdList)
{
    return s_gigeFactory.getDeviceList(outIdList);
}

bool GigeCamera::openCamera(QString cameraID)
{
    bool ret = true;
    int64_t int64Val;
    J_STATUS_TYPE retval;
    closeCamera();
    retval = J_Camera_Open(s_gigeFactory.hFactory, (int8_t*)cameraID.toLatin1().data(), &p_d->hCamera);
    if (retval != J_ST_SUCCESS) {
        qDebug()<<"Could not open the camera!"<<retval;
        ret = false;
        return ret;
    }

    // Get Width from the camera based on GenICam name
    retval = J_Camera_GetValueInt64(p_d->hCamera, NODE_NAME_WIDTH, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<QString("Could not get Width value! ret:")<<retval;
    }
    p_d->width = (int)int64Val;

    // Get Height from the camera
    retval = J_Camera_GetValueInt64(p_d->hCamera, NODE_NAME_HEIGHT, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<QString("Could not get Height value! ret:")<<retval;
    }
    p_d->height = (int)int64Val;

    // Get the pixelformat from the camera
    int64_t pixelFormat = 0;
    uint64_t jaiPixelFormat = 0;
    retval = J_Camera_GetValueInt64(p_d->hCamera, NODE_NAME_PIXELFORMAT, &pixelFormat);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"Unable to get PixelFormat value!"<<retval;
    } else {
        J_Image_Get_PixelFormat(p_d->hCamera, pixelFormat, &jaiPixelFormat);
    }
    // Calculate number of bits (not bytes) per pixel using macro
    int bpp = J_BitsPerPixel(jaiPixelFormat);
    uint32_t streamCount = 0;
    J_Camera_GetNumOfDataStreams(p_d->hCamera, &streamCount);
    p_d->streamCount = streamCount;
    this->cameraID = cameraID;
    qDebug()<<QString("open camera %1 success, width %2 height: %3, stramCount: %4, pixelFormat: %5 pixelBpp: %6")
              .arg(cameraID).arg(p_d->width).arg(p_d->height).arg(p_d->streamCount).arg(jaiPixelFormat).arg(bpp);
    return ret;
}

bool GigeCamera::configCamera()
{
    J_STATUS_TYPE retVal;
    DEV_HANDLE hParent = 0;
    if (p_d->hCamera == NULL) {
        qWarning()<<"must open camera before configCamera.";
        return false;
    }
    retVal = J_Camera_GetLocalDeviceHandle(p_d->hCamera, &hParent);
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

bool GigeCamera::startPlayStream(int streamNum, bool useHistogram)
{
    J_STATUS_TYPE retval;
    uint32_t buffSize = p_d->width * p_d->height * 6;
    // Open stream
    retval = J_Image_OpenStream(p_d->hCamera, (uint32_t)streamNum, p_d
                                , reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&GigeCameraBasePrivate::gigeDecodeCallback)
                                , &p_d->hThread, buffSize);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"J_Image_OpenStream failed."<<retval;
        return false;
    }
    // Acquisition Start
    if (J_Camera_ExecuteCommand(p_d->hCamera, NODE_NAME_ACQSTART) != J_ST_SUCCESS) {
        qWarning()<<"AcquisitionStart failed.";
        return false;
    }
    p_d->t.start();
    p_d->useHistogram = useHistogram;
    return true;
}

bool GigeCamera::stopPlayStream()
{
    J_STATUS_TYPE retval;
    if (p_d->hCamera) {
        retval = J_Camera_ExecuteCommand(p_d->hCamera, NODE_NAME_ACQSTOP);
        if (retval != J_ST_SUCCESS) {
            qWarning()<<"Could not Stop Acquisition!"<<retval;
        }
    }
    if (p_d->hThread) {
        retval = J_Image_CloseStream(p_d->hThread);
        if (retval != J_ST_SUCCESS) {
            qWarning()<<"CloseStream error!"<<retval;
        }
        p_d->hThread = NULL;
    }
    p_d->t.invalidate();

    if (p_d->YBufferInfo.pImageBuffer) {
        J_Image_Free(&p_d->YBufferInfo);
        p_d->YBufferInfo.pImageBuffer = NULL;
    }
    if (p_d->BufferInfo.pImageBuffer) {
        J_Image_Free(&p_d->BufferInfo);
        p_d->BufferInfo.pImageBuffer = NULL;
    }
    if (p_d->histogramInfo.iColors != 0) {
         J_Image_FreeHistogram(&p_d->histogramInfo);
         memset(&p_d->histogramInfo, 0, sizeof(p_d->histogramInfo));
    }
    return true;
}

bool GigeCamera::getFrame(JaiGSFrame &frame, QList<int> *redHis, QList<int> *greenHis, QList<int> *blueHis)
{
    if ((p_d->frameValid.elapsed() > 5000) || p_d->frame.data.isEmpty()) {
        return false;
    }
    frame = p_d->frame;
    p_d->frame.data.clear();
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
bool GigeCamera::setFramegrabberValue(const QString name, int64_t int64Val)
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
bool GigeCamera::SetFramegrabberPixelFormat(QString name, int64_t jaiPixelFormat)
{
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

    int8_t szJaiPixelFormatName[512];
    uint32_t iSize = 512;
    retval = J_Image_Get_PixelFormatName(p_d->hCamera, jaiPixelFormat, szJaiPixelFormatName, iSize);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get pixelFormat, ret"<<retval;
        return false;
    }

    NODE_HANDLE hLocalDeviceNode = 0;
    retval = J_Camera_GetNodeByName(hDev, (int8_t *)"PixelFormat", &hLocalDeviceNode);
    if (J_ST_SUCCESS != retval) {
        qWarning()<<"cannot get pixelFormat, ret"<<retval;
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
        J_STATUS_TYPE retval = J_Camera_GetNodeByName(hDev, (int8_t*)strIncoming.toLatin1().data(), &hNodeIncoming);
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

void GigeCamera::dumpCameraCategoryNode()
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

bool GigeCamera::setSize(int width, int height)
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

bool GigeCamera::getGain(int& minGain, int& maxGain, int& currentGain)
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

bool GigeCamera::setGain(int gain)
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

bool GigeCamera::getExposureTime(int& minExposure, int& maxExposure, int& exposure)
{
    J_STATUS_TYPE   retval;
    int64_t         int64Val;

    if (p_d->hExposureNode == NULL) {
        retval = J_Camera_GetNodeByName(p_d->hCamera, NODE_NAME_EXPOSURE, &p_d->hExposureNode);
        if (retval != J_ST_SUCCESS) {
            p_d->hExposureNode = NULL;
            qWarning()<<"cannot get node handel. "<<NODE_NAME_EXPOSURE<<retval;
            return false;
        }
        // Get/Set Min
        retval = J_Node_GetMinInt64(p_d->hExposureNode, &int64Val);
        p_d->minExposure = int64Val;
        // Get/Set Max
        retval = J_Node_GetMaxInt64(p_d->hExposureNode, &int64Val);
        p_d->maxExposure = int64Val;
    }

    minExposure = (int)p_d->minExposure;
    maxExposure = (int)p_d->maxExposure;

    // Get/Set Value
    retval = J_Node_GetValueInt64(p_d->hExposureNode, TRUE, &int64Val);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"cannot get current gain. ret"<<retval;
        return false;
    }
    exposure = (int)int64Val;
    return true;
}

bool GigeCamera::setExposureTime(int exposure)
{
    J_STATUS_TYPE   retval;
    // We have two possible ways of setting up Exposure time: JAI or GenICam SFNC
    // The JAI Exposure time setup uses a node called "ShutterMode" and the SFNC
    // does not need to set up anything in order to be able to control the exposure time.
    // Therefor we have to determine which way to use here.
    // First we see if a node called "ShutterMode" exists.
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

    int minExposure, maxExposure, currentExposure;
    if (!getExposureTime(minExposure, maxExposure, currentExposure)) {
         return false;
    }

    if ((exposure > maxExposure) || (exposure < minExposure)) {
        qWarning()<<QString("the exposure out of the range (%1,%2)").arg(minExposure).arg(maxExposure);
        return false;
    }

    retval = J_Node_SetValueInt64(p_d->hExposureNode, TRUE, (int64_t)exposure);
    if (retval != J_ST_SUCCESS) {
        qWarning()<<"set exposure node value "<<exposure<<" failed. "<<retval;
        return false;
    }
    return true;
}
