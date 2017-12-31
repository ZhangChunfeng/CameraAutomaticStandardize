#include <QDebug>
#include <QTimer>
#include <QRect>
#include <QEventLoop>
#include "hkNVRSrvDevice.h"
#include "ptzHY0.h"
#include "ptzPelcoD.h"

HKNVRSrvDevice::HKNVRSrvDevice(QObject *parent)
    : QObject(parent)
    , HKNVRDriver()
    , PTZDriver()
{
    m_transChanTime.start();
}

HKNVRSrvDevice::~HKNVRSrvDevice()
{

}

bool HKNVRSrvDevice::getDeviceReply(unsigned char *buff, int len)
{
    if (m_serialPortData.count() < len) {
        return false;
    }
//    len = qMin(len, m_serialPortData.count());
    memcpy(buff, m_serialPortData.constData(), len);
    return true;
}

bool HKNVRSrvDevice::zoomWide(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_ZOOM_IN, isStop);
}

bool HKNVRSrvDevice::zoomNarrow(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_ZOOM_OUT, isStop);
}

bool HKNVRSrvDevice::focusFar(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_FOCUS_FAR, isStop);
}

bool HKNVRSrvDevice::focusNear(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_FOCUS_NEAR, isStop);
}

bool HKNVRSrvDevice::IrisClose(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_IRIS_CLOSE, isStop);
}

bool HKNVRSrvDevice::IrisOpen(bool isStop)
{
    return HKNVRDriver::ptzControl(HKPTZCMD_IRIS_OPEN, isStop);
}

QByteArray HKNVRSrvDevice::getRecvData()
{
    return m_serialPortData;
}

void HKNVRSrvDevice::loginFinished()
{
    emit hkDevLoginResult(isLogined());
}

bool HKNVRSrvDevice::stop()
{
    if (isLogined()) {
        return sendData(m_PTZProtocol->stop());
    } else {
        return true;
    }
}

bool HKNVRSrvDevice::up(int speed)
{
    return sendData(m_PTZProtocol->up(speed));
}

bool HKNVRSrvDevice::down(int speed)
{
    return sendData(m_PTZProtocol->down(speed));
}

bool HKNVRSrvDevice::left(int speed)
{
    return sendData(m_PTZProtocol->left(speed));
}

bool HKNVRSrvDevice::right(int speed)
{
    return sendData(m_PTZProtocol->right(speed));
}

bool HKNVRSrvDevice::leftDown(int speedH, int speedV)
{
    return sendData(m_PTZProtocol->leftDown(speedH, speedV));
}

bool HKNVRSrvDevice::rightDown(int speedH, int speedV)
{
    return sendData(m_PTZProtocol->rightDown(speedH, speedV));
}

bool HKNVRSrvDevice::leftUp(int speedH, int speedV)
{
    return sendData(m_PTZProtocol->leftUp(speedH, speedV));
}

bool HKNVRSrvDevice::rightUp(int speedH, int speedV)
{
    return sendData(m_PTZProtocol->rightUp(speedH, speedV));
}

bool HKNVRSrvDevice::lightOn()
{
    return sendData(m_PTZProtocol->lightOn());
}

bool HKNVRSrvDevice::lightOff()
{
    return sendData(m_PTZProtocol->lightOff());
}

bool HKNVRSrvDevice::wiperOn()
{
    return sendData(m_PTZProtocol->wiperOn());
}

bool HKNVRSrvDevice::wiperOff()
{
    return sendData(m_PTZProtocol->wiperOff());
}

bool HKNVRSrvDevice::gotoPresetPos(int id, int angleH, int angleV)
{
    QRect rangRect(PTZhy0::MIN_ANGLE_H, PTZhy0::MIN_ANGLE_V,
                        PTZhy0::MAX_ANGLE_H - PTZhy0::MIN_ANGLE_H,
                        PTZhy0::MAX_ANGLE_V - PTZhy0::MIN_ANGLE_V);
    if ((m_PTZProtocol->getName() == PRTO_NAME_HY0) &&
        rangRect.contains(QPoint(angleH, angleV))) {
        m_serialPortData.clear();
        if (!sendData(m_PTZProtocol->angleCtrl(angleH, angleV))) {
            return false;
        }
        return true;
//        QTimer timer;
//        timer.setInterval(5000);
//        QEventLoop evLoop;
//        connect(this, &HKNVRSrvDevice::replyDataArrived, &evLoop, &QEventLoop::quit);
//        connect(&timer, &QTimer::timeout, [&evLoop](){
//            evLoop.exit(-1);
//        });
//        timer.start();
//        if (evLoop.exec() == -1) {
//            qDebug()<<" , Error: no reply data arrived";
//            return false;
//        }
    } else {
        if (!sendData(m_PTZProtocol->gotoPresetPos(char(id)))) {
            return false;
        }
//        QEventLoop ev;
//        QTimer::singleShot(3000, &ev, &QEventLoop::quit);  //  强制等待3000 ms，等云台运动到位。
//        ev.exec();
    }

    return true;
}

bool HKNVRSrvDevice::setPresetPos(int id)
{
    bool ret = sendData(m_PTZProtocol->setPresetPos(char(id)));

    if (m_PTZProtocol->getName() == PRTO_NAME_HY0) {
        if (ret) {
            QTimer::singleShot(100, [this]{
                m_serialPortData.clear();
                sendData(m_PTZProtocol->angleQuery());
            });
        }

        QEventLoop ev;
        QTimer::singleShot(1000, &ev, &QEventLoop::quit);
        connect(this, &HKNVRSrvDevice::replyDataArrived, &ev, &QEventLoop::quit);
        ev.exec();
    }
    return ret;
}

bool HKNVRSrvDevice::clearPresetPos(int id)
{
    return sendData(m_PTZProtocol->clearPresetPos(char(id)));
}

void HKNVRSrvDevice::transChanDataArrived(const char* buff, int size)
{
    qint64 elapsedMs = m_transChanTime.elapsed();
    m_transChanTime.restart();
    if (elapsedMs < 0 || elapsedMs > 60     // 60ms以内的数据数据视为连续数据
        || m_serialPortData.count() >= 100) {
        m_serialPortData.clear();
    }
    m_serialPortData += QByteArray(buff, size);
    if (m_PTZProtocol->getName() == PRTO_NAME_HY0) {
        if (m_serialPortData.count() >= 12) {
            qDebug()<<"transChanData Recv: "<<m_serialPortData.toHex();
            emit replyDataArrived();
        }
    }
}
