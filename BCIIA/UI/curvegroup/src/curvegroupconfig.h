#ifndef CurveGroupConfig_H
#define CurveGroupConfig_H

#include "QSettings"
class CurveGroupConfig
{
public:
    CurveGroupConfig();
    static void init();
    static uint8_t getGroupNum();
    static uint16_t getMaxPoint();
};

#endif // CurveGroupConfig_H
