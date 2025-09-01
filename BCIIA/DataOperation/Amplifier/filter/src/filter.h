#ifndef FILTER_H
#define FILTER_H
#include "QList"
#include "iirfilter.h"
class  Filter
{
public:
    Filter();
    void init();
    QList<double> filterData(QList<double> data);
    void setChannelNum(uint8_t);
    void setSampleRate(uint16_t rate);
    void setMuscleIndex(const QList<uint8_t> &value);

    void setBreathIndex(const QList<uint8_t> &value);

    void setHeartIndex(const QList<uint8_t> &value);

    void setEEGIndex(const QList<uint8_t> &value);

    void setEEGGameIndex(const QList<uint8_t> &value);

private:
    IIRFILTER *iirfilter;
    uint8_t channel_num;

    QList<uint8_t> muscleIndex;
    QList<uint8_t> breathIndex;
    QList<uint8_t> heartIndex;
    QList<uint8_t> EEGIndex;
    QList<uint8_t> EEGGameIndex;

    double * high_b;
    double * high_a;
    double * low_b;
    double * low_a;
};

#endif // FILTER_H
