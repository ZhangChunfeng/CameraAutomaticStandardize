#ifndef HKNVRDRIVER_H
#define HKNVRDRIVER_H

#include <QList>
#include <QVector>
#include <QString>
#include <QFile>
#include <QReadWriteLock>
#include <QByteArray>
#include <QRunnable>
#include <qt_windows.h>
#include <QRgb>
#include <QPoint>
#include "plaympeg4.h"

#define MAX_QUEUE_LEN   2

#define GSFRAME_TYPE_YV12     1
#define GSFRAME_TYPE_RGB888   2
#define GSFRAME_TYPE_INDEX8   4

#define GSFRAME_TYPE_DISABLE_DISPLAY 0x10

#define BUFF_WIDTH  1920
#define BUFF_HEIGHT 1080
#define BUFFSIZE  ((BUFF_WIDTH)*(BUFF_HEIGHT)*3)

typedef struct HKGSFrameTag {
    long nWidth;
    long nHeight;
    long nStamp;
    long nFrameRate;
    long dwFrameNum;
    int  nType;

    QByteArray data;
    QVector<QRgb>  colorTable;
    int intMaxTemperature;
    int intMinTemperature;
    QPoint maxPos;
    QPoint minPos;
}HKGSFrame;

typedef struct VideoChanTag {
    QString         name;
    int             chanNum;
    bool            isPlaying;
    long            realHandle;
    long            decodePort;
    QList<HKGSFrame>  frames;
    VideoChanTag() {
        chanNum = -1;
        isPlaying = false;
        realHandle = -1;
        decodePort = -1;
    }
    VideoChanTag(int channelNum): chanNum(channelNum){
        isPlaying = false;
        realHandle = -1;
        decodePort = -1;
    }
    bool operator == (const struct VideoChanTag& other) {
        bool ret = false;
        if (chanNum > 0) {
            ret |= chanNum == other.chanNum;
        }
        if (realHandle >= 0) {
            ret |= realHandle == other.realHandle;
        }
        if (decodePort >= 0) {
            ret |= decodePort == other.decodePort;
        }
        return ret;
    }
}VideoChan;

typedef struct ChannelInfoTag {
    QString name;
    int     chanNum;
    bool    isEnable;
    ChannelInfoTag() {
        chanNum = -1;
        isEnable = false;
    }
}ChannelInfo;

typedef struct DeviceInfoTag {
    int                 devType;            // 设备类型
    int                 analogChanCount;    // 模拟通道数
    int                 analogStarChan;     // 模拟起始通道
    int                 iIPChanNum;         // 最大数字通道个数
    int                 iIPStartChan;       // 数字通道起始通道号
    QString             serialNumber;
    QString             devName;
    QList<ChannelInfo>  chanInfo;

    DeviceInfoTag() {
        devType         = -1;
        analogChanCount =  0;
        analogStarChan	= 0;
        iIPChanNum      = 0;
        iIPStartChan    = 0;
    }
}DeviceInfo;

#define HKPTZCMD_LIGHT_PWRON           2     //接通灯光电源
#define HKPTZCMD_WIPER_PWRON           3     //接通雨刷开关
#define HKPTZCMD_FAN_PWRON             4     //接通风扇开关
#define HKPTZCMD_HEATER_PWRON          5     //接通加热器开关
#define HKPTZCMD_AUX_PWRON1            6     //接通辅助设备开关
#define HKPTZCMD_AUX_PWRON2            7     //接通辅助设备开关
#define HKPTZCMD_ZOOM_IN               11    //焦距变大(倍率变大)
#define HKPTZCMD_ZOOM_OUT              12    //焦距变小(倍率变小)
#define HKPTZCMD_FOCUS_NEAR            13    //焦点前调
#define HKPTZCMD_FOCUS_FAR             14    //焦点后调
#define HKPTZCMD_IRIS_OPEN             15    //光圈扩大
#define HKPTZCMD_IRIS_CLOSE            16    //光圈缩小
#define HKPTZCMD_TILT_UP               21    //云台上仰
#define HKPTZCMD_TILT_DOWN             22    //云台下俯
#define HKPTZCMD_PAN_LEFT              23    //云台左转
#define HKPTZCMD_PAN_RIGHT             24    //云台右转
#define HKPTZCMD_UP_LEFT               25    //云台上仰和左转
#define HKPTZCMD_UP_RIGHT              26    //云台上仰和右转
#define HKPTZCMD_DOWN_LEFT             27    //云台下俯和左转
#define HKPTZCMD_DOWN_RIGHT            28    //云台下俯和右转
#define HKPTZCMD_PAN_AUTO              29    //云台左右自动扫描
#define HKPTZCMD_TILT_DOWN_ZOOM_IN     58    //云台下俯和焦距变大(倍率变大)
#define HKPTZCMD_TILT_DOWN_ZOOM_OUT    59    //云台下俯和焦距变小(倍率变小)
#define HKPTZCMD_PAN_LEFT_ZOOM_IN      60    //云台左转和焦距变大(倍率变大)
#define HKPTZCMD_PAN_LEFT_ZOOM_OUT     61    //云台左转和焦距变小(倍率变小)
#define HKPTZCMD_PAN_RIGHT_ZOOM_IN     62    //云台右转和焦距变大(倍率变大)
#define HKPTZCMD_PAN_RIGHT_ZOOM_OUT    63    //云台右转和焦距变小(倍率变小)
#define HKPTZCMD_UP_LEFT_ZOOM_IN       64    //云台上仰和左转和焦距变大(倍率变大)
#define HKPTZCMD_UP_LEFT_ZOOM_OUT      65    //云台上仰和左转和焦距变小(倍率变小)
#define HKPTZCMD_UP_RIGHT_ZOOM_IN      66    //云台上仰和右转和焦距变大(倍率变大)
#define HKPTZCMD_UP_RIGHT_ZOOM_OUT     67    //云台上仰和右转和焦距变小(倍率变小)
#define HKPTZCMD_DOWN_LEFT_ZOOM_IN     68    //云台下俯和左转和焦距变大(倍率变大)
#define HKPTZCMD_DOWN_LEFT_ZOOM_OUT    69    //云台下俯和左转和焦距变小(倍率变小)
#define HKPTZCMD_DOWN_RIGHT_ZOOM_IN    70    //云台下俯和右转和焦距变大(倍率变大)
#define HKPTZCMD_DOWN_RIGHT_ZOOM_OU    71    //云台下俯和右转和焦距变小(倍率变小)
#define HKPTZCMD_TILT_UP_ZOOM_IN       72    //云台上仰和焦距变大(倍率变大)
#define HKPTZCMD_TILT_UP_ZOOM_OUT      73    //云台上仰和焦距变小(倍率变小)

class HKNVRDriver;

class AsyncLoginRunnable : public QRunnable
{
public:
    AsyncLoginRunnable(HKNVRDriver* hkNvr,
                       const QString& ip, long port,
                       const QString& user, const QString& pwd);
protected:
    void run();
private:
    HKNVRDriver* m_hkNvr;
    QString      m_ip;
    long         m_port;
    QString      m_user;
    QString      m_pwd;
    QWriteLocker m_writeLocker;
};

class HKNVRDriver
{
public:
    HKNVRDriver();
    virtual ~HKNVRDriver();
    static QString getLastError();
    static bool cleanup();
    //注册登陆
    bool    syncdoLogin(const QString& ip, long port, const QString& user, const QString& pwd);
    void    asyncLogin(const QString& ip, long port, const QString& user, const QString& pwd);
    bool    isConnecting();
    virtual void loginFinished();
    bool    isLogined();
    void    doLogout();
    void    exitLogin();
    DeviceInfo getDeviceInfo();
    bool    setDateTime(const QDateTime& dateTime);
    bool    setExposureTime(int us);
    int     getExposureTime();
    // 获取图像模块
    bool    getFrame(int chanNum, HKGSFrame& frame);
    bool	stopPlay(int chanNum);
    bool	startPlay(int chanNum, int stream_type = 0);
    QByteArray capturePicToMEM(int chanNum);
    bool    savePic(QByteArray data, const QString& fileName);
    bool    capturePicture(int chanNum, const QString& fileName);
    bool    captureJpegPicture(int chanNum, const QString& fileName);
    bool    startRecord(int chanNum, const QString&  fileName);
    bool    stopRecord(int chanNum);
    VideoChan getPlayingChan(int chanNum);
    bool    isPlayingChan(int chanNum);

    // 云台控制
    bool    ptzControl(UINT cmd, bool isStop);
    void    setEnableSerialCmd(bool enable);
    // 透明通道模块
    bool    startTranChan();
    bool    stopTansChan();
    bool    configTranChan(int chanNum, int baudRatio);
    bool    sendData(char* buff, int size);
    bool    sendData(QByteArray message);
    virtual void transChanDataArrived(const char* buff, int size);

    QReadWriteLock      m_lock;
protected:
    long                lLoginID;  //-1表示未登录状态；
    DeviceInfo          deviceInfo;
    QByteArray          m_serialPortData;
    bool                m_enableSerialCmd;

private:
    long getDecodePort(long realHandle);
    void setDecodePort(long realHandle, long decodePort);
    void frameArrived(long port, const HKGSFrame& frame);
    void doGetChannalCfg();

private:
    long                lTranHandle;
    QList<VideoChan>    m_playingChanList;
    int                 connectionWatchDog;
    QString             m_ipAddr;
    bool                m_isConnecting;
    char                m_captruteImageBuff[BUFFSIZE];
private:
    //初始化海康SDK
    static void initHK();
    static void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser);
    static void CALLBACK DecCBFun(long nPort, char * pBuf, long nSize,
                                  FRAME_INFO * pFrameInfo, long nUser, long nReserved2);
    static void CALLBACK RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType,
                                              BYTE *pBuffer, DWORD dwBufSize, void *pUser);
    static void CALLBACK SerialDataCallBack(LONG lSerialHandle, char* pRecvDataBuffer,
                                            DWORD dwBufSize, DWORD dwUser);
};

#endif // HKNVRDRIVER_H
