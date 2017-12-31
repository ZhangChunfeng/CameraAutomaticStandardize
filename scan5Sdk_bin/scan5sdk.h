#ifndef SCAN5SDK_H
#define SCAN5SDK_H

#include "scan5sdk_global.h"

class SdkInnerData;

#include <stdint.h>

typedef void (*DataCallBack)(uint16_t* data, int width, uint16_t rotatSpeed, uint16_t temp, void* user);

class SCAN5SDKSHARED_EXPORT Scan5SDK
{
public:
    Scan5SDK();
    ~Scan5SDK();

    bool connectTo(QString ipAddr);
    void startSend();
    void stopSend();
    void close();
    void setDataHandCallBack(DataCallBack func, void*param);
private:
    SdkInnerData* d;
};

#endif // SCAN5SDK_H
