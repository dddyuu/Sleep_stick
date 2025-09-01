#ifndef EEGDATA_H
#define EEGDATA_H


#include "QObject"
#include "QMutex"
class EEGData
{
public:
    EEGData();
    ~EEGData();
    void clear();
    void append(QList<double>,QList<double>);
    QList<QList<double>> getChartData(int channel_num);
    QList<double> getRawData();
private:
    QMutex mutex;
    QList<double> chart_data;
    QList<double> raw_data;

};

#endif // EEGDATA_H
