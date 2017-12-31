#include <QDebug>
#include <QDateTime>
#include <QException>
#include <QThreadPool>
#include <QCoreApplication>
#include "hkNVRDriver.h"
#include "HCNetSDK.h"
#include <QElapsedTimer>

HKNVRDriver::HKNVRDriver()
    : lLoginID(-1)
    , lTranHandle(-1)
    , connectionWatchDog(10)
    , m_enableSerialCmd(false)
    , m_isConnecting(false)
{
    initHK();
    NET_DVR_SetExceptionCallBack_V30(WM_NULL, NULL, &HKNVRDriver::ExceptionCallBack, this);
}

HKNVRDriver::~HKNVRDriver()
{
    for (int i = 0; i < m_playingChanList.count(); i++) {
        stopPlay(m_playingChanList.at(i).chanNum);
    }
    QElapsedTimer t;
    t.start();
    while(m_isConnecting && (t.elapsed() < 15000)) {
        QCoreApplication::processEvents();
    }
    if (t.elapsed() > 14990) {
        qCritical()<<"HKNVRDrvier cannot destroy Normally.  use time "<<t.elapsed()<<m_isConnecting;
    }
}

QString HKNVRDriver::getLastError()
{
    LONG err = NET_DVR_GetLastError();
    QString error = QString("last error %1: ").arg(err) + QByteArray(NET_DVR_GetErrorMsg(&err));
    return error;
}

bool HKNVRDriver::cleanup()
{
    return NET_DVR_Cleanup();
}

bool HKNVRDriver::startTranChan()
{
    //建立透明通道
    int iSelSerialIndex = 2; //1:232串口；2:485串口
    lTranHandle = NET_DVR_SerialStart(lLoginID, iSelSerialIndex, &HKNVRDriver::SerialDataCallBack, (DWORD)this);//设置回调函数获取透传数据
    if (lTranHandle < 0) {
        qCritical()<<"start Serial port error. " + getLastError();
        return false;
    }
    return true;
}

bool HKNVRDriver::sendData(char* buff, int size)
{
    if (!m_enableSerialCmd) {
        return false;
    }
    //通过透明通道发送数据
    LONG lSerialChan = 0; //使用485时该值有效，从1开始；232时设置为0
    if (lTranHandle < 0) {
        startTranChan();
    }

    if (!NET_DVR_SerialSend(lTranHandle, lSerialChan, buff, size)) {
        qCritical()<<QString("NET_DVR_SerialSend error, %1 ").arg(getLastError());
        stopTansChan();
        return false;
    }
    return true;
}

bool HKNVRDriver::sendData(QByteArray message)
{
    if (message.count() <= 0) {
        return true;
    }
    QString str;
    foreach (uchar byte, message) {
        str += QString().sprintf("0x%02X ", byte);
    }
    qDebug()<<"hk NVRDriver send msg: ";
    qDebug()<<str<<endl;
    return sendData(message.data(), message.size());
}

bool HKNVRDriver::stopTansChan()
{
    //断开透明通道
    if (lTranHandle < 0) {
        return true;
    }
    if (NET_DVR_SerialStop(lTranHandle)) {
        lTranHandle = -1;
        return true;
    }
    qWarning()<<"NET_DVR_SerialStop error: " << getLastError();
    return false;
}

void HKNVRDriver::transChanDataArrived(const char* buff, int size)
{
    m_serialPortData += QByteArray(buff, size);
//    qDebug()<<m_serialPortData.toHex();
}

bool HKNVRDriver::configTranChan(int chanNum, int baudRatio)
{
    const int baudList[] = {
        50,   75,   110,   150,
        300,  600,  1200,  2400,
        4800, 9600, 19200, 38400,
        57600,76800,115200
    };
    //设置RS485透明通道波特率
    DWORD dwReturned = 0;
    NET_DVR_DECODERCFG_V30 struDecodeCfg;
    memset(&struDecodeCfg, 0, sizeof(NET_DVR_DECODERCFG_V30));
    if (lLoginID < 0) {
        return false;
    }
    if (!NET_DVR_GetDVRConfig(lLoginID, NET_DVR_GET_DECODERCFG_V30,
                              chanNum, &struDecodeCfg, sizeof(NET_DVR_DECODERCFG_V30), &dwReturned)) {
        qCritical()<<"configChan baudRatio error: "<<getLastError();
        return false;
    }

    int count = sizeof(baudList)/sizeof(int);
    for (int i = 0; i < count; i++) {
        if (baudList[i] >= baudRatio) {
            struDecodeCfg.dwBaudRate = i;
            break;
        }
    }
    if (!NET_DVR_SetDVRConfig(lLoginID, NET_DVR_SET_DECODERCFG_V30, chanNum, &(struDecodeCfg),
                              sizeof(NET_DVR_DECODERCFG_V30))) {
        qCritical()<<QString("NET_DVR_SET_DECODERCFG_V30 error, %1.").arg(getLastError());
        return false;
    }
    return true;
}

long HKNVRDriver::getDecodePort(long realHandle)
{
    VideoChan dummy;
    dummy.realHandle = realHandle;
    int index = m_playingChanList.indexOf(dummy);
    if (index >= 0) {
        return m_playingChanList.at(index).decodePort;
    }
    return -1;
}

void HKNVRDriver::setDecodePort(long realHandle, long decodePort)
{
    VideoChan dummy;
    dummy.realHandle = realHandle;
    int index = m_playingChanList.indexOf(dummy);
    if (index >= 0) {
        m_playingChanList[index].decodePort = decodePort;
        qDebug() << "set the decode port sucess... handel: "<<realHandle<<" port: "<<decodePort;
        return;
    }
    qWarning() << "error: cannot set the decode port... handel: "<<realHandle<<" port: "<<decodePort;
}

/*************************************************
函数名:        getFrame
函数描述:    获取一帧图片
输入参数:
输出参数:
返回值:        成功与否
**************************************************/
bool HKNVRDriver::getFrame(int chanNum, HKGSFrame& frame)
{
    bool ret = false;
    VideoChan dummy;
    dummy.chanNum = chanNum;
    if (!m_lock.tryLockForRead()) {
        return false;
    }
    int index = m_playingChanList.indexOf(dummy);
    if (index >= 0) {
        if (!m_playingChanList.at(index).frames.isEmpty()) {
            frame = m_playingChanList.at(index).frames.first();
            ret = true;
        }
    }
    m_lock.unlock();
    return ret;
}

void HKNVRDriver::frameArrived(long port, const HKGSFrame& frame)
{
    VideoChan dummy;
    dummy.decodePort = port;
    int index = m_playingChanList.indexOf(dummy);
    if (index >= 0) {
        m_lock.lockForWrite();
        m_playingChanList[index].frames.append(frame);
        if (m_playingChanList.at(index).frames.count() > MAX_QUEUE_LEN) {
            m_playingChanList[index].frames.removeFirst();
        }
        m_lock.unlock();
    }
}

DeviceInfo HKNVRDriver::getDeviceInfo()
{
    return deviceInfo;
}

bool HKNVRDriver::setDateTime(const QDateTime& dateTime)
{
    NET_DVR_TIME t;
    t.dwYear = dateTime.date().year();
    t.dwMonth = dateTime.date().month();
    t.dwDay = dateTime.date().day();
    t.dwHour = dateTime.time().hour();
    t.dwMinute = dateTime.time().minute();
    t.dwSecond = dateTime.time().second();
    if (lLoginID > 0) {
        if (!NET_DVR_SetDVRConfig(lLoginID, NET_DVR_SET_TIMECFG,
                                  0xFFFFFFFF, &t, sizeof(t))) {
            qCritical()<<getLastError();
            return false;
        } else {
            return true;
        }
    }
    return false;
}

bool HKNVRDriver::setExposureTime(int us)
{
    if (lLoginID < 0) {
        return false;
    }
    NET_DVR_CAMERAPARAMCFG param;
    DWORD sizeRt = 0;
    if (NET_DVR_GetDVRConfig(lLoginID, NET_DVR_GET_CCDPARAMCFG, 0xFFFFFFFF, &param, sizeof(param), &sizeRt)) {
        param.struVideoEffect.byBrightnessLevel = us;
        qDebug()<<"=========================================";
        qDebug()<<"byAutoApertureLevel is "<<param.struExposure.byAutoApertureLevel;
        qDebug()<<"byExposureMode is "<<param.struExposure.byExposureMode;
        qDebug()<<"dwExposureUserSet is "<<param.struExposure.dwExposureUserSet;
        qDebug()<<"dwVideoExposureSet is "<<param.struExposure.dwVideoExposureSet;
        if (NET_DVR_SetDVRConfig(lLoginID, NET_DVR_SET_CCDPARAMCFG, 0xFFFFFFFF, &param, sizeof(param))) {
            return true;
        } else {
            qCritical()<<QString("NET_DVR_SET_CCDPARAMCFG error, %1.").arg(getLastError());
        }
    } else {
        qCritical()<<QString("NET_DVR_GetDVRConfig NET_DVR_GET_CCDPARAMCFG error, %1.").arg(getLastError());
    }
    return false;
}

int HKNVRDriver::getExposureTime()
{
    if (lLoginID < 0) {
        return -1;
    }
    NET_DVR_CAMERAPARAMCFG param;
    DWORD sizeRt = 0;
    if (NET_DVR_GetDVRConfig(lLoginID, NET_DVR_GET_CCDPARAMCFG, 0xFFFFFFFF, &param, sizeof(param), &sizeRt)) {
        qDebug()<<"=========================================";
        qDebug()<<"byAutoApertureLevel is "<<param.struExposure.byAutoApertureLevel;
        qDebug()<<"byExposureMode is "<<param.struExposure.byExposureMode;
        qDebug()<<"dwExposureUserSet is "<<param.struExposure.dwExposureUserSet;
        qDebug()<<"dwVideoExposureSet is "<<param.struExposure.dwVideoExposureSet;
    } else {
        qCritical()<<QString("NET_DVR_GetDVRConfig NET_DVR_GET_CCDPARAMCFG error, %1.").arg(getLastError());
    }
    return 0;
}

/*************************************************
函数名:    	DoGetDeviceResoureCfg
函数描述:	获取设备的通道资源
输入参数:
输出参数:
返回值:		void
**************************************************/
void HKNVRDriver::doGetChannalCfg()
{
    NET_DVR_IPPARACFG_V40 IpAccessCfg;
    memset(&IpAccessCfg, 0, sizeof(IpAccessCfg));
    DWORD  dwReturned = 0;

    bool ret = NET_DVR_GetDVRConfig(lLoginID, NET_DVR_GET_IPPARACFG_V40, 0,
                                    &IpAccessCfg, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    qDebug()<<"ipAccessCfg-size: "<<IpAccessCfg.dwSize;
    qDebug()<<"ipAccessCfg-groupNum: "<<IpAccessCfg.dwGroupNum;
    qDebug()<<"ipAccessCfg-analogChan: "<<IpAccessCfg.dwAChanNum;
    qDebug()<<"ipAccessCfg-digtalChan: "<<IpAccessCfg.dwDChanNum;
//    qDebug()<<"ipAccessCfg-analogEn: "<<IpAccessCfg.byAnalogChanEnable;
    qDebug()<<"ipAccessCfg-startIP: "<<IpAccessCfg.dwStartDChan;
    deviceInfo.chanInfo.clear();
    ChannelInfo info;
    if (!ret) {
        qDebug()<<"cannot find the ip config "<<getLastError();
        //不支持ip接入,9000以下设备不支持禁用模拟通道
        for (int i = 0; i < MAX_ANALOG_CHANNUM; i++) {
            if (i < deviceInfo.analogChanCount) {
                info.name    = QString("Camera%1").arg(i);
                info.chanNum = deviceInfo.analogStarChan + i;  //通道号
                info.isEnable = true;
                deviceInfo.chanInfo.append(info);
            }
        }
    } else {       //支持IP接入，9000设备
        for(int i = 0; i < MAX_ANALOG_CHANNUM; i++) { //接入IP的模拟通道
            if (i < deviceInfo.analogChanCount) {
                info.name = QString("Camera%1").arg(i);
                info.chanNum = deviceInfo.analogStarChan + i;  //通道号
                if (IpAccessCfg.byAnalogChanEnable[i]) {
                    info.isEnable = true;
                } else {
                    info.isEnable = false;
                }
                deviceInfo.chanInfo.append(info);
            }
        }

        //数字通道
        for (int i = 0; i < MAX_IP_CHANNEL; i++) {
            if (IpAccessCfg.struStreamMode[i].uGetStream.struChanInfo.byEnable) {  //ip通道在线
                info.chanNum  = i + IpAccessCfg.dwStartDChan;
                info.name = QString("IP Camera%1").arg(i + 1);
                info.isEnable = true;
                deviceInfo.chanInfo.append(info);
            }
        }
    }
    qDebug()<<"device name: "<<deviceInfo.devName;
    qDebug()<<"device serial: "<<deviceInfo.serialNumber;
    qDebug()<<"valid count: "<<deviceInfo.chanInfo.count();
    for (int i = 0; i < deviceInfo.chanInfo.count(); i++) {
        qDebug()<<"index:   "<<i;
        qDebug()<<"name:  "<<deviceInfo.chanInfo.at(i).name;
        qDebug()<<"chanNum: "<<deviceInfo.chanInfo.at(i).chanNum;
        qDebug()<<"enable:  "<<deviceInfo.chanInfo.at(i).isEnable;
    }
}

/*************************************************
函数名:      DoLogin
函数描述:    向设备登录注册
输入参数:
输出参数:
返回值:        成功与否
**************************************************/
bool HKNVRDriver::syncdoLogin(const QString& ip, long port, const QString& user, const QString& pwd)
{
    m_isConnecting = true;
    m_ipAddr = ip;
    if (lLoginID != -1) {
        exitLogin();
    }

    NET_DVR_DEVICEINFO_V30 struDeviceInfo;
    memset(&struDeviceInfo, 0, sizeof(NET_DVR_DEVICEINFO_V30));
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
    struLoginInfo.bUseAsynLogin = false;
    struLoginInfo.wPort = port;
    int count = qMin<long>(qstrlen(ip.toLatin1().constData()), NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sDeviceAddress, ip.toLatin1().constData(), count);

    count = qMin<long>(qstrlen(user.toLatin1().constData()), NAME_LEN);
    memcpy(struLoginInfo.sUserName, user.toLatin1().constData(), count);

    count = qMin<long>(qstrlen(pwd.toLatin1().constData()), PASSWD_LEN);
    memcpy(struLoginInfo.sPassword, pwd.toLatin1().constData(), count);

    lLoginID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
    if (lLoginID < 0) {
        if (NET_DVR_GetLastError() == NET_DVR_PASSWORD_ERROR) {
            qWarning()<<ip<<QStringLiteral(" 用户名或密码错误!");
            if (struDeviceInfoV40.bySupportLock == 1) {
                qWarning()<<ip<<QString(QStringLiteral(" 还剩%1尝试机会"))
                            .arg(struDeviceInfoV40.byRetryLoginTime);
            }
        } else if (NET_DVR_GetLastError() == NET_DVR_USER_LOCKED) {
            if(struDeviceInfoV40.bySupportLock == 1) {
                qWarning()<<ip<<QString(QStringLiteral(" IP被锁定，剩余锁定时间%1秒"))
                            .arg(struDeviceInfoV40.dwSurplusLockTime);
            }
        } else {
            qWarning()<<ip<<QString(QStringLiteral(" 由于网络原因或DVR忙, 在设备%1上注册失败!")).arg(ip);
        }
        loginFinished();
        m_isConnecting = false;
        return false;
    } else {
        if (struDeviceInfoV40.byPasswordLevel == 1) {
            qWarning()<<ip<<QStringLiteral(" 默认密码，请修改!");
        } else if(struDeviceInfoV40.byPasswordLevel == 3) {
            qWarning()<<ip<<QStringLiteral(" 风险密码，请修改!");
        }
        memcpy(&struDeviceInfo, &struDeviceInfoV40.struDeviceV30, sizeof(struDeviceInfo));
    }

    deviceInfo.devType         = struDeviceInfo.wDevType;
    deviceInfo.analogChanCount = struDeviceInfo.byChanNum;
    deviceInfo.analogStarChan  = struDeviceInfo.byStartChan;
    deviceInfo.iIPChanNum      = struDeviceInfo.byIPChanNum + struDeviceInfo.byHighDChanNum * 256;
    deviceInfo.iIPStartChan    = struDeviceInfo.byStartDChan;
    deviceInfo.serialNumber    = QByteArray((char*)struDeviceInfo.sSerialNumber, SERIALNO_LEN);

    DWORD dwReturned = 0;
    NET_DVR_DEVICECFG_V40 struDevCfg;
    if (!NET_DVR_GetDVRConfig(lLoginID, NET_DVR_GET_DEVICECFG_V40, 0xFFFFFFFF,
                              &struDevCfg, sizeof(NET_DVR_DEVICECFG_V40), &dwReturned)) {
        qCritical()<<ip<<" get nvr deviceConfig failed. "<<getLastError();
    } else {
        deviceInfo.devName = QByteArray((char*)struDevCfg.byDevTypeName, DEV_TYPE_NAME_LEN);
    }

    doGetChannalCfg();

    qDebug()<<ip<<" HKNVRDriver login in ok. "<<ip;
    loginFinished();
    m_isConnecting = false;
    return true;
}

void HKNVRDriver::asyncLogin(const QString& ip, long port, const QString& user, const QString& pwd)
{
    if (!m_isConnecting) {
        AsyncLoginRunnable* loginRunnable = new AsyncLoginRunnable(this, ip, port, user, pwd);
        QThreadPool::globalInstance()->start(loginRunnable);
    }
}

bool HKNVRDriver::isConnecting()
{
    return m_isConnecting;
}

void HKNVRDriver::loginFinished()
{
    qDebug()<<"login finished.";
}

bool HKNVRDriver::isLogined()
{
    return lLoginID >= 0;
}
/*************************************************
函数名:        DoLogout
函数描述:    设备注销
输入参数:
输出参数:
返回值:        void
**************************************************/
void HKNVRDriver::doLogout()
{
    for (int i = 0; i < m_playingChanList.count(); i++) {
        stopPlay(m_playingChanList.at(i).chanNum);
    }
    stopTansChan();
    NET_DVR_Logout(lLoginID);
    deviceInfo = DeviceInfo();
    qDebug()<<QString("HKNVRDriver lLoginID:%1 logout ok. ").arg(lLoginID);
    lLoginID = -1;
}

//该接口强制停止该用户的所有操作和释放所有的资源，确保该ID对应的线程都安全退出，资源得到释放;
void HKNVRDriver::exitLogin()
{
    NET_DVR_Logout_V30(lLoginID);
    lLoginID = -1;
    deviceInfo = DeviceInfo();
    qDebug()<<QString("HKNVRDriver lLoginID:%1 exit ok. ").arg(lLoginID);
}

/*************************************************
函数名:        StartPlay
函数描述:    开始一路播放
输入参数:   ChanIndex-通道号
输出参数:
返回值:        成功与否
**************************************************/
bool HKNVRDriver::startPlay(int chanNum, int stream_type)
{
    bool ret = false;
    //NET_DVR_RealPlay_V40////////////////////////////
    NET_DVR_PREVIEWINFO struPreviewInfo;
    memset(&struPreviewInfo, 0, sizeof(struPreviewInfo));
    struPreviewInfo.hPlayWnd        = NULL;        //CALLBACK mode
    struPreviewInfo.lChannel        = chanNum;
    struPreviewInfo.dwStreamType    = stream_type; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    struPreviewInfo.dwLinkMode      = 0;           // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    struPreviewInfo.bBlocked        = 0;           //1 阻塞；    0 非阻塞
    struPreviewInfo.byPreviewMode   = 0;           //预览模式，0-正常预览，1-延迟预览
    struPreviewInfo.byProtoType     = 0;           //应用层取流协议，0-私有协议，1-RTSP协议

    VideoChan dummy;
    dummy.chanNum = chanNum;
    int index = m_playingChanList.indexOf(dummy);
    if (index >= 0) {
        if (m_playingChanList.at(index).isPlaying) { // 已经在预览状态
            return true;
        } else {
            stopPlay(chanNum);
        }
    }

    VideoChan chan;
    chan.chanNum = chanNum;
    chan.realHandle = NET_DVR_RealPlay_V40(lLoginID, &struPreviewInfo, &HKNVRDriver::RealDataCallBack_V30, this);
//    qDebug() <<" handel "<<chan.realHandle<<chanNum;
    if (chan.realHandle < 0) {
        qCritical()<<QString("start play chan %1 error: %2 ").arg(chanNum).arg(getLastError());
        ret = false;
    } else {
        chan.isPlaying = true;
        m_playingChanList.append(chan);
        ret = true;
    }
    return ret;
}

/*************************************************
函数名:        capturePicToMEM
函数描述:      抓取图片到内存中
输入参数:      ChanIndex-通道号
输出参数:
返回值:        截取的图片信息
**************************************************/
QByteArray HKNVRDriver::capturePicToMEM(int chanNum)
{
    if (lLoginID < 0) {
        return QByteArray();
    }
    NET_DVR_JPEGPARA strujpegInfo;
    memset(&strujpegInfo, 0, sizeof(strujpegInfo));
    strujpegInfo.wPicSize = 0xff;      //使用当前码流分辨率
    strujpegInfo.wPicQuality = 0;      //图片质量系数：0-最好，1-较好，2-一般
    QByteArray data;
    DWORD sizeReturned = 0;            //返回图片大小
    try {
        if (NET_DVR_CaptureJPEGPicture_NEW(lLoginID, chanNum, &strujpegInfo, m_captruteImageBuff,
                                           BUFFSIZE, &sizeReturned)){
//            qDebug()<<"NET_DVR_CaptureJPEGPicture_NEW, time elapsed (ms): "<< t.elapsed();
            data = QByteArray(m_captruteImageBuff, sizeReturned);
        } else {
            qWarning()<<QString("%1 capture picture chan %2 error:").arg(m_ipAddr).arg(chanNum)
                        + getLastError();
        }
    } catch (...) {
        qCritical()<<QString("%1 NET_DVR_CaptureJPEGPicture_NEW Exception: catch a exception may be no memory ")
                     .arg(m_ipAddr);
    }

    return data;

}

/*************************************************
函数名:        savePic
函数描述:      将内存中的图片保存到本地磁盘中
输入参数:      data-内存中的图片信息，fileName-保存的文件路径及名称
**************************************************/
bool HKNVRDriver::savePic(QByteArray data, const QString &fileName)
{
    QFile file(fileName);
    if(! file.open(QFile::WriteOnly)){
        qCritical()<<"File can't open!";
        return false;
    }
    qint64 s = file.write(data);
    if(s == -1){
       qCritical()<<" An error occurred while writing";
       return false;
    }
    return true;
}

bool HKNVRDriver::capturePicture(int chanNum, const QString &fileName)
{
//    LONG realHandle = getPlayingChan(chanNum).realHandle;
//    if (realHandle < 0) {
//        qDebug()<<QString("chanNum %1 isn't playing !").arg(chanNum);
//        return false;
//    }
//    bool ret = NET_DVR_CapturePicture(realHandle, fileName.toLocal8Bit().data());
//    if (!ret) {
//        qDebug()<<getLastError();
//    }
//    return ret;

    return savePic(capturePicToMEM(chanNum), fileName);
}

bool HKNVRDriver::captureJpegPicture(int chanNum, const QString &fileName)
{
    if (lLoginID < 0) {
        qWarning()<<  QString("hkDevice didn't login !").arg(chanNum);
        return false;
    }
    NET_DVR_JPEGPARA strujpegInfo;
    strujpegInfo.wPicSize = 0xff;      //使用当前码流分辨率
    strujpegInfo.wPicQuality = 0;      //图片质量系数：0-最好，1-较好，2-一般
//    QElapsedTimer t;
//    t.start();
    bool ret = NET_DVR_CaptureJPEGPicture(lLoginID, chanNum, & strujpegInfo, fileName.toLocal8Bit().data());
//    qDebug()<<"NET_DVR_CaptureJPEGPicture, time elapsed (ms): "<< t.elapsed();
    if (!ret) {
        qCritical()<< getLastError();
    }
    return ret;
}

bool HKNVRDriver::startRecord(int chanNum, const QString &fileName)
{
    LONG realHandle = getPlayingChan(chanNum).realHandle;
    if (realHandle < 0) {
        qWarning()<<  QString("chanNum %1 isn't playing !").arg(chanNum);
        return false;
    }
    bool ret = NET_DVR_SaveRealData(realHandle, fileName.toLocal8Bit().data()); // 连续调用时，会自动停止上一次录像；
    if (!ret) {
        qCritical()<< getLastError();
    }
    return ret;
}

bool HKNVRDriver::stopRecord(int chanNum)
{
    LONG realHandle = getPlayingChan(chanNum).realHandle;
    if (realHandle < 0) {
        qCritical()<<  QString("chanNum %1 isn't playing !").arg(chanNum);
        return false;
    }
    bool ret = NET_DVR_StopSaveRealData(realHandle);
    if (!ret) {
        qCritical()<< getLastError();
    }
    return ret;
}

VideoChan HKNVRDriver::getPlayingChan(int chanNum)
{
//    VideoChan chan;
//    int i = m_playingChanList.size();
//    while (--i >= 0) {
//        chan = m_playingChanList.at(i);
//        if (chan.chanNum == chanNum) {
//            return chan;
//        }
//    }
//    return VideoChan();

    int ind = m_playingChanList.indexOf(VideoChan(chanNum));
    return m_playingChanList.value(ind);
}

bool HKNVRDriver::isPlayingChan(int chanNum)
{
    return m_playingChanList.contains(VideoChan(chanNum));
}

bool HKNVRDriver::ptzControl(UINT cmd, bool isStop)
{
    if (!m_enableSerialCmd) {
        return false;
    }
    bool ret = NET_DVR_PTZControl_Other(lLoginID, 1, cmd, isStop? 1 : 0);
    if (!ret) {
        qCritical() << "error: "<<getLastError();
    }
    return ret;
}

void HKNVRDriver::setEnableSerialCmd(bool enable)
{
    m_enableSerialCmd = enable;
}

/*************************************************
函数名:        StopPlay
函数描述:    停止播放
输入参数:
输出参数:
返回值:        成功与否
**************************************************/
bool HKNVRDriver::stopPlay(int chanNum)
{
    bool ret = true;

    VideoChan dummy;
    dummy.chanNum = chanNum;
    int index = m_playingChanList.indexOf(dummy);
    if (index < 0) {
        return false;
    }
    long handle = m_playingChanList.at(index).realHandle;
    if (handle >= 0 ) {
        if (NET_DVR_StopRealPlay(handle)) {
            m_playingChanList[index].isPlaying = false;
            ret = true;
        } else {
            ret = false;
        }
        long port = m_playingChanList.at(index).decodePort;
        PlayM4_Stop(port);
        PlayM4_CloseStream(port);
        PlayM4_FreePort(port);
        m_playingChanList.removeAt(index);
    }
    return ret;
}

/*************************************************
函数名:        initHK
函数描述:    初始化海康SDK
输入参数:
输出参数:
返回值:
**************************************************/
void HKNVRDriver::initHK()
{
    static bool isInited = false;
    BOOL bRet = false;
    if (isInited) {
        return;
    }
    bRet = NET_DVR_Init();
    bRet &= NET_DVR_SetConnectTime(3000,1);
    bRet &= NET_DVR_SetReconnect(10000);
    isInited = bRet;
}

void HKNVRDriver::DecCBFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nUser, long nReserved2)
{
    Q_UNUSED(nReserved2);
    if (!nUser) {
        return;
    }
    HKGSFrame frame;
    HKNVRDriver* obj = (HKNVRDriver*)nUser;
    switch (pFrameInfo->nType)
    {
    case T_AUDIO16:
        break;
    case T_UYVY:
        break;
    case T_YV12:
        frame.nWidth     = pFrameInfo->nWidth;
        frame.nHeight    = pFrameInfo->nHeight;
        frame.nStamp     = pFrameInfo->nStamp;
        frame.dwFrameNum = pFrameInfo->dwFrameNum;
        frame.nFrameRate = pFrameInfo->nFrameRate;
        frame.nType      = GSFRAME_TYPE_YV12;
        frame.data       = QByteArray(pBuf, nSize);
        obj->frameArrived(nPort, frame);
        break;
    case T_RGB32:
        break;
    default:
        break;
    }
}

// 实时流回调
void HKNVRDriver::RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
    HKNVRDriver* obj = (HKNVRDriver*)(pUser);
    LONG    nPort;
    if (!obj || (lRealHandle < 0)) {
        return;
    }
    nPort = obj->getDecodePort(lRealHandle);
    DWORD dRet = 0;
    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD:
//        qDebug()<<"here .... NET_DVR_SYSHEAD ";
        if (!PlayM4_GetPort(&nPort)) {  //获取播放库未使用的通道号
            break;
        } else {
            obj->setDecodePort(lRealHandle, nPort);
        }
        if (dwBufSize > 0 && nPort >= 0) {
            if (!PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }

            if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1024*1024)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }

            // 设置流解码回调函数
            if (!PlayM4_SetDecCallBackMend(nPort, &HKNVRDriver::DecCBFun, (long)pUser)) {
                printf("Decode CallBack Set Error!\n");
                dRet = PlayM4_GetLastError(nPort);
                break;
            }

            // 打开视频解码
            if (!PlayM4_Play(nPort, NULL)) {
                dRet = PlayM4_GetLastError(nPort);
                break;
            }
        }
        break;

    case NET_DVR_STREAMDATA:
//        qDebug()<<"here .... input data NET_DVR_STREAMDATA";
        if (dwBufSize > 0 && nPort >= 0) {
            PlayM4_InputData(nPort, pBuffer, dwBufSize);
        }
        break;
    default:
        if (dwBufSize > 0 && nPort >= 0) {
            qDebug()<<"here ..also.. input data NET_DVR_STREAMDATA";
            PlayM4_InputData(nPort, pBuffer,dwBufSize);
        }
        break;
    }
    if (dRet != 0) {
        qDebug()<<"RealDataCallBack_V30 error: code "<<dRet;
    }
}

void HKNVRDriver::ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    Q_UNUSED(lUserID);
    Q_UNUSED(lHandle);
    Q_UNUSED(pUser);
    char tempbuf[256];
    ZeroMemory(tempbuf,256);
    switch(dwType) {
        case EXCEPTION_EXCHANGE:
            qWarning()<<"============= EXCEPTION_EXCHANGE"
                    <<QStringLiteral("用户交互时异常（注册心跳超时，心跳间隔为2分钟）");
            break;
        case EXCEPTION_AUDIOEXCHANGE:
            qWarning()<<"============= EXCEPTION_AUDIOEXCHANGE"
                    <<QStringLiteral("语音对讲异常");
            break;
        case EXCEPTION_ALARM:
            qWarning()<<"============= EXCEPTION_ALARM"
                    <<QStringLiteral("报警异常");
            break;
        case EXCEPTION_PREVIEW:
            qWarning()<<"============= EXCEPTION_PREVIEW"
                    <<QStringLiteral("网络预览异常");
            break;
        case EXCEPTION_SERIAL:
            qWarning()<<"============= EXCEPTION_SERIAL"<<QStringLiteral("透明通道异常");
            break;
        case EXCEPTION_RECONNECT:
            qWarning()<<"============= EXCEPTION_RECONNECT"<<QStringLiteral("预览时重连");
            break;
        case EXCEPTION_ALARMRECONNECT:
            qWarning()<<"============= EXCEPTION_ALARMRECONNECT "<<QStringLiteral("报警时重连");
            break;
        case EXCEPTION_SERIALRECONNECT:
            qWarning()<<"============= EXCEPTION_SERIALRECONNECT "<<QStringLiteral("透明通道重连");
            break;
        case SERIAL_RECONNECTSUCCESS:
            qWarning()<<"============= SERIAL_RECONNECTSUCCESS "<<QStringLiteral("透明通道重连成功");
            break;
        case EXCEPTION_PLAYBACK:
            qWarning()<<"============= EXCEPTION_PLAYBACK "<<QStringLiteral("回放异常");
            break;
        case EXCEPTION_DISKFMT:
            qWarning()<<"============= EXCEPTION_DISKFMT "<<QStringLiteral("硬盘格式化");
            break;
        case EXCEPTION_PASSIVEDECODE:
            qWarning()<<"============= EXCEPTION_PASSIVEDECODE "<<QStringLiteral("被动解码异常");
            break;
        case EXCEPTION_EMAILTEST:
            qWarning()<<"============= EXCEPTION_EMAILTEST "<<QStringLiteral("邮件测试异常");
            break;
        case EXCEPTION_BACKUP:
            qWarning()<<"============= EXCEPTION_BACKUP "<<QStringLiteral("备份异常");
            break;
        case PREVIEW_RECONNECTSUCCESS:
            qWarning()<<"============= PREVIEW_RECONNECTSUCCESS "<<QStringLiteral("预览时重连成功");
            break;
        case ALARM_RECONNECTSUCCESS:
            qWarning()<<"============= ALARM_RECONNECTSUCCESS "<<QStringLiteral("报警时重连成功");
            break;
        case RESUME_EXCHANGE:
            qWarning()<<"============= RESUME_EXCHANGE "<<QStringLiteral("用户交互恢复");
            break;
        case NETWORK_FLOWTEST_EXCEPTION:
            qWarning()<<"============= NETWORK_FLOWTEST_EXCEPTION "<<QStringLiteral("网络流量检测异常");
            break;
        case EXCEPTION_PICPREVIEWRECONNECT:
            qWarning()<<"============= EXCEPTION_PICPREVIEWRECONNECT "<<QStringLiteral("图片预览重连");
            break;
        case PICPREVIEW_RECONNECTSUCCESS:
            qWarning()<<"============= PICPREVIEW_RECONNECTSUCCESS "<<QStringLiteral("图片预览重连成功");
            break;
        case EXCEPTION_PICPREVIEW:
            qWarning()<<"============= EXCEPTION_PICPREVIEW "<<QStringLiteral("图片预览异常");
            break;
        case EXCEPTION_MAX_ALARM_INFO:
            qWarning()<<"============= EXCEPTION_MAX_ALARM_INFO "<<QStringLiteral("报警信息缓存已达上限");
            break;
        case EXCEPTION_LOST_ALARM:
            qWarning()<<"============= EXCEPTION_LOST_ALARM "<<QStringLiteral("报警丢失");
            break;
        case EXCEPTION_PASSIVETRANSRECONNECT:
            qWarning()<<"============= EXCEPTION_PASSIVETRANSRECONNECT "<<QStringLiteral("被动转码重连");
            break;
        case PASSIVETRANS_RECONNECTSUCCESS:
            qWarning()<<"============= PASSIVETRANS_RECONNECTSUCCESS "<<QStringLiteral("被动转码重连成功");
            break;
        case EXCEPTION_PASSIVETRANS:
            qWarning()<<"============= EXCEPTION_PASSIVETRANS "<<QStringLiteral("被动转码异常");
            break;
        case SUCCESS_PUSHDEVLOGON:
            qWarning()<<"============= SUCCESS_PUSHDEVLOGON "<<QStringLiteral("推模式设备注册成功");
            break;
        case EXCEPTION_RELOGIN:
            qWarning()<<"============= EXCEPTION_RELOGIN "<<QStringLiteral("用户重登陆");
            break;
        case RELOGIN_SUCCESS:
            qWarning()<<"============= SUCCESS_PUSHDEVLOGON "<<QStringLiteral("用户重登陆成功");
            break;
        default:
            qWarning()<<"=============== unknown EXCEPTION: "<<(void*)(dwType);
            break;
    }
    LONG err = NET_DVR_GetLastError();
    qWarning()<<QString("last error %1: ").arg(err) + QByteArray(NET_DVR_GetErrorMsg(&err));
}

void HKNVRDriver::SerialDataCallBack(LONG lSerialHandle, char* pRecvDataBuffer, DWORD dwBufSize, DWORD dwUser)
{
    Q_UNUSED(lSerialHandle);
    HKNVRDriver* obj =(HKNVRDriver*)dwUser;
//    qDebug()<<"SerialDataCallBack ...."<<QByteArray(pRecvDataBuffer, dwBufSize);
    if (obj) {
        obj->transChanDataArrived(pRecvDataBuffer, dwBufSize);
    }
}


AsyncLoginRunnable::AsyncLoginRunnable(HKNVRDriver* hkNvr
, const QString& ip, long port, const QString& user, const QString& pwd)
    : m_hkNvr(hkNvr)
    , m_ip(ip)
    , m_port(port)
    , m_user(user)
    , m_pwd(pwd)
    , m_writeLocker(&hkNvr->m_lock)
{
}

void AsyncLoginRunnable::run()
{
    if (m_hkNvr) {
//        qDebug()<<"====== async login begin:";
        m_hkNvr->syncdoLogin(m_ip, m_port, m_user, m_pwd);
//        qDebug()<<"====== async login end:";
    }
}
