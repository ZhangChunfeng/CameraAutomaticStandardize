#ifndef HKNVRSRVDEVICE_H
#define HKNVRSRVDEVICE_H

#include <QObject>
#include <QElapsedTimer>
#include "hkNVRDriver.h"
#include "ptzDriver.h"
#include "PTZProtocol.h"

class HKNVRSrvDevice : public QObject, public HKNVRDriver, public PTZDriver
{
    Q_OBJECT
public:
    HKNVRSrvDevice(QObject* parent);
    virtual ~HKNVRSrvDevice();

    // 云台控制层
    bool up(int speed);
    bool down(int speed);
    bool left(int speed);
    bool right(int speed);
    bool leftDown(int speedH, int speedV);
    bool rightDown(int speedH, int speedV);
    bool leftUp(int speedH, int speedV);
    bool rightUp(int speedH, int speedV);
    bool gotoPresetPos(int id, int angleH, int angleV);
    bool setPresetPos(int id);
    bool clearPresetPos(int id);
    bool stop();
    bool getDeviceReply(unsigned char* buff, int len);
    bool lightOn();
    bool lightOff();
    bool wiperOn();
    bool wiperOff();

    bool zoomWide(bool isStop);
    bool zoomNarrow(bool isStop);
    bool focusFar(bool isStop);
    bool focusNear(bool isStop);
    bool IrisClose(bool isStop);
    bool IrisOpen(bool isStop);
    QByteArray getRecvData();
signals:
    void replyDataArrived();
    void hkDevLoginResult(bool isLogined);

private:
    void loginFinished();
    void transChanDataArrived(const char* buff, int size);
private:
    QElapsedTimer m_transChanTime;
};

#endif // HKNVRSRVDEVICE_H
