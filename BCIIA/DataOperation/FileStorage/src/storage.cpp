#include "storage.h"
void Storage::setFilename(QString name)
{
    this->name=name;
    number=0;
}

QString Storage::getFilename()
{
    return name;
}

void Storage::getNumber(quint16 &number)
{
    number=this->number;this->number++;
}

void Storage::save(double *data, int num)
{
    point_num+=num;
}

void Storage::save(double *, int eeg, double *, int heart)
{
   eeg_num+=eeg;
   heart_num+=heart;
}

void Storage::setChanlocs(QVariantList chanlocs)
{
    this->chanlocs=chanlocs;
}

QVariantList Storage::getChanlocs()
{
    return chanlocs;
}

void Storage::setSrate(quint16 rate)
{
    this->srate=rate;
}

quint16 Storage::getSrate()
{
    return srate;
}

void Storage::stop()
{
    point_num=0;
    eeg_num=0;
    heart_num=0;
    event_id=0;
}

void Storage::appendEvent(int type,int num)
{
    event_id++; //事件序号累加
    QVariantMap map;
    map["latency"]=point_num+num;
    map["duration"]=1;
    map["channel"]=0;
    map["bvmknum"]=int(event_id);
    map["type"]=type;
    map["code"]="Stimulus";
    //原始脑电事件插入
    urevent.append(map);
    map["urevent"]=int(event_id);
    //脑电事件插入
    event.append(map);

}

QVariantList Storage::getEvent() const
{
    return event;
}

QVariantList Storage::getUrevent() const
{
    return urevent;
}

void Storage::clearEvent()
{
    event.clear();
    urevent.clear();
}

int Storage::getPoint_num() const
{
    return point_num;
}

int Storage::getEEG_num() const
{
    return eeg_num;
}

int Storage::getHeart_num() const
{
    return heart_num;
}
