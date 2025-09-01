#include "matread.h"
#include "QDir"
#include "QDebug"
#include "QtConcurrent"
#include "QDebug" 

MatRead::MatRead(QObject *parent) : QObject(parent)
{
    //#ifdef HAVE_HDF5
    //qDebug() << "HDF5 support is enabled." ;
    //#else
    //qDebug() << "HDF5 support is not enabled.";
    //#endif
}
QList<QList<double>> MatRead::readlocalmat(QString dir)
{
  
    mat_t* mat = Mat_Open(dir.toStdString().data(), MAT_ACC_RDONLY);
    if (mat == NULL) {
        // 处理错误，文件无法打开
        qDebug() << "无法打开文件:" << dir;
        return QList<QList<double>>();
    }
    matvar_t* eeg = Mat_VarRead(mat, "EEG");
    if(eeg == NULL)
    {
        qDebug() << "have no 'EEG'";
    }
    //qDebug() << "EEG structure fields:";
    //int fieldCount = Mat_VarGetNumberOfFields(eeg);
    //// 获取所有字段名的字符串数组
    //char* const* fieldnames = Mat_VarGetStructFieldnames(eeg);
    //// 遍历所有字段
    //for (unsigned i = 0; i < fieldCount; ++i) {
    //    qDebug() << "Field" << i << ":" << fieldnames[i];

    //    // 如果需要访问字段内容
    //    matvar_t* field = Mat_VarGetStructFieldByIndex(eeg, i, 0);
    //    if (field) {
    //        qDebug() << "  Type:" << field->data_type
    //            << "Class:" << field->class_type
    //            << "Dimensions:" << field->dims[0] << "x" << field->dims[1];
    //        Mat_VarFree(field);
    //    }
    //}
    


    matvar_t *lData=Mat_VarGetStructFieldByName(eeg,"eeg",0);
//    matvar_t* ecg = Mat_VarRead(mat, "ECG");
    if (lData == NULL) {
        // 处理错误，变量不存在
        qDebug() << "have no 'eeg'";
        Mat_VarFree(lData);
        Mat_VarFree(eeg);
        Mat_Close(mat);
        return QList<QList<double>>();
    }

    //数据读取
    //获取数据的维度(通道数,采样点)
    size_t* dim = lData->dims;
    int channael_num = dim[0];
    int point_num = dim[1];
//    int channael_num = dim[1]; // 原为dim[0]
//    int point_num = dim[0];     // 原为dim[1]
    qDebug() << channael_num;
    qDebug() << point_num;
    double* data = (double*)lData->data;
    QList<QList<double>> eegData;
    for (int ch = 0; ch < channael_num; ++ch) {
        QList<double> channelData;
        for (int samp = 0; samp < point_num; ++samp) {
            channelData.append(getexpand*data[samp * channael_num + ch]);
            
        }
        eegData.append(channelData);
    }
    Mat_VarFree(eeg);
    Mat_Close(mat);

    return eegData;
}
QList<uint8_t> MatRead::readlocallabel(QString dir) {
    mat_t* mat = Mat_Open(dir.toStdString().data(), MAT_ACC_RDONLY);
    if (mat == NULL) {
        // 处理错误，文件无法打开
        qDebug() << "无法打开文件:" << dir;
        return QList<uint8_t>();
    }
    matvar_t* eeg = Mat_VarRead(mat, "EEG");
    if (eeg == NULL)
    {
        qDebug() << "have no 'EEG'";
    }
    matvar_t* lLabel = Mat_VarGetStructFieldByName(eeg, "label", 0);
    if (lLabel == NULL) {
        // 处理错误，变量不存在
        qDebug() << "have no 'label'";
        Mat_VarFree(lLabel);
        Mat_VarFree(eeg);
        Mat_Close(mat);
        return QList<uint8_t>();
    }
    //获取数据的维度(标签数,标签)
    size_t* dim = lLabel->dims;
    int labeln = dim[0];
    qDebug() << labeln;
    uint8_t* data = (uint8_t*)lLabel->data;
    QList<uint8_t> labelData;
    for (int ch = 0; ch < labeln; ++ch) {

        labelData.append(data[ch]);
    }
    Mat_VarFree(eeg);
    Mat_Close(mat);

    return labelData;

}





void MatRead::setDir(QString dir)
{
    QDir  matdir(dir);
    if(!matdir.exists())
    {
        qDebug()<<"目录错误";
        QCoreApplication::exit();
    }
    else
    {
        this->dir=dir;
        filesname=matdir.entryList(QDir::Files);
        QStringList name_element=filesname[0].split("_");
        QString filename;
        for (int i = 0; i < name_element.size()-1; i++)
        {
            filename+=name_element[i]+"_";
        }
        filename=filename.remove(filename.size()-1,filename.size());
        this->filename=dir+"/"+filename+".mat";
//        qDebug()<<this->filename;
        read();
        write();
        qDebug()<<"数据合并成功\n路径为:"<<this->filename;
    }

}

void MatRead::write()
{
    //创建合并的文件
    mat_t *mat=Mat_Create(filename.toStdString().data(),NULL);
    //写入文件名字
    writeFilename();
    //写入导联信息
    Mat_VarSetStructFieldByName(eeg,"chanlocs",0,chanlocs);
    //写入脑电数据
    matvar_t *data=Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dim,eegdata,0);
    Mat_VarSetStructFieldByName(eeg,"data",0,data);
    //写入事件
    writeEvent();
    //写入原始事件
    writeUrevent();
    //写入采样率
    Mat_VarSetStructFieldByName(eeg,"srate",0,srate);
    //将上述数据保存到合并文件
    Mat_VarWrite(mat,eeg,MAT_COMPRESSION_NONE);
    //关闭文件
    Mat_Close(mat);
    //释放内存
    Mat_VarFree(eeg);
    qDeleteAll(events);
    qDeleteAll(urevents);
    events.clear();
    urevents.clear();
//    qDebug()<<"结束";
}
void MatRead::read()
{
    size_t dim[2]={1,1};
    const char *field[9]={"chanlocs","data","srate","filename","event","urevent","ref","age","set"};
    this->eeg=Mat_VarCreateStruct("EEG",2,dim,field,9);
    for(auto file_path:filesname)
    {
        file_path=dir+"/"+file_path;
        if(file_path.contains(".mat"))
        {
            mat_t *mat = Mat_Open(file_path.toStdString().data(), MAT_ACC_RDONLY);
            if(mat==NULL)
            {
                qDebug()<<"文件打开失败";
            }
            else
            {
                matvar_t *eeg=Mat_VarRead(mat,"EEG");
                readData(eeg);
                readLabel(eeg);
                readevent(eeg);
                readUrevent(eeg);
                readSrate(eeg);
                Mat_VarFree(eeg);
                Mat_Close(mat);
            }
        }


    }

}

void MatRead::readData(matvar_t *eeg)
{
    //数据读取
    matvar_t *data=Mat_VarGetStructFieldByName(eeg,"data",0);
    //获取数据的维度(通道数,采样点)
    size_t *dim=data->dims;
    int channael_num=dim[0];
    int point_num=dim[1];
    int data_length=channael_num*point_num;
    memcpy(eegdata_end,data->data,data_length*sizeof(double));
    //尾指针向后移动
    eegdata_end+=data_length;
}

void MatRead::setDataDim(int channel_num,int point_num)
{
    dim[0]=channel_num;
    dim[1]=point_num;
    quint64 _size=channel_num*point_num*sizeof (double);
    eegdata=(double*)malloc(_size);
    eegdata_end=eegdata;
}

void MatRead::readLabel(matvar_t *eeg)
{
    if(chanlocs==NULL)
    {
        matvar_t *channels=Mat_VarGetStructFieldByName(eeg,"chanlocs",0);
        int channel_num=channels->dims[0];
        const char *chanlocs_item_field[12]={
            "labels","type","theta","radius",
            "x","y","z","sph_theta","sph_phi",
            "sph_radius","urchan","ref"
        };
        size_t dim[2]={1,1};
        dim[0]=channel_num;
        chanlocs=Mat_VarCreateStruct("chanlocs",2,dim,chanlocs_item_field,12);
        for(int i=0;i<channel_num;i++)
        {
            for(int j=0;j<12;j++)
            {
                matvar_t *chanlocs_item=Mat_VarGetStructFieldByName(channels,chanlocs_item_field[j],i);
                if(chanlocs_item->dims[0]!=0)
                {
                    size_t field_dim[2]={1,1};
                    if(chanlocs_item->class_type==MAT_C_CHAR)
                    {
                       field_dim[1]=chanlocs_item->nbytes;
                    }
                    char *field_data=(char*)(chanlocs_item->data);

                    matvar_t *field=Mat_VarCreate(NULL,chanlocs_item->class_type,chanlocs_item->data_type,2,field_dim,field_data,0);
                    Mat_VarSetStructFieldByName(chanlocs,chanlocs_item_field[j],i,field);
                }
            }

        }
    }
}

void MatRead::readevent(matvar_t *eeg)
{
    const char *event_item_field[8]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code","urevent"
    };
    //获取当前文件的event字段数据
    matvar_t *events=Mat_VarGetStructFieldByName(eeg,"event",0);
    int event_num=events->dims[0];
    for(int i=0;i<event_num;i++)
    {
        //获取单个事件
        size_t event_dims[2]={1,1};
        matvar_t *event=Mat_VarCreateStruct(NULL,2,event_dims,event_item_field,8);
        for(int j=0;j<8;j++)
        {
            //获取单个事件的字段
            matvar_t *event_item=Mat_VarGetStructFieldByName(events,event_item_field[j],i);
            if(event_item!=NULL&&event_item->dims[0]!=0)
            {
                size_t field_dim[2]={1,1};
                if(event_item->class_type==MAT_C_CHAR)
                {
                   field_dim[1]=event_item->nbytes;
                }
                char *field_data=(char*)(event_item->data);
                matvar_t *field=Mat_VarCreate(NULL,event_item->class_type,event_item->data_type,2,field_dim,field_data,0);
                Mat_VarSetStructFieldByName(event,event_item_field[j],0,field);
            }
        }
        this->events.append(event);
    }
}

void MatRead::readUrevent(matvar_t *eeg)
{
    const char *event_item_field[7]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code"
    };
    //获取当前文件的event字段数据
    matvar_t *urevents=Mat_VarGetStructFieldByName(eeg,"urevent",0);
    int event_num=urevents->dims[0];
    for(int i=0;i<event_num;i++)
    {
        //获取单个事件
        size_t event_dims[2]={1,1};
        matvar_t *event=Mat_VarCreateStruct(NULL,2,event_dims,event_item_field,7);
        for(int j=0;j<7;j++)
        {
            //获取单个事件的字段
            matvar_t *event_item=Mat_VarGetStructFieldByName(urevents,event_item_field[j],i);
            if(event_item!=NULL&&event_item->dims[0]!=0)
            {
                size_t field_dim[2]={1,1};
                if(event_item->class_type==MAT_C_CHAR)
                {
                   field_dim[1]=event_item->nbytes;
                }
                char *field_data=(char*)(event_item->data);
                matvar_t *field=Mat_VarCreate(NULL,event_item->class_type,event_item->data_type,2,field_dim,field_data,0);
                Mat_VarSetStructFieldByName(event,event_item_field[j],0,field);
            }
        }
        this->urevents.append(event);
    }
}

void MatRead::readSrate(matvar_t *eeg)
{
    if(srate==NULL)
    {
         //读取数据的采样率
         matvar_t *srate=Mat_VarGetStructFieldByName(eeg,"srate",0);
         if(srate!=NULL)
         {
           //将采样率保存
           this->srate=Mat_VarCreate(NULL,srate->class_type,srate->data_type,2,srate->dims,srate->data,0);
         }
    }
}

void MatRead::writeFilename()
{
    QFileInfo info(filename);
    QString name=info.fileName();
    int len=name.size();
    size_t dim[2];
    dim[0]=1;
    dim[1]=len;
    matvar_t *filename_field=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dim,name.toUtf8().data(),0);
    Mat_VarSetStructFieldByName(eeg,"filename",0,filename_field);
}

void MatRead::writeEvent()
{
    const char *event_item_field[8]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code","urevent"
    };
    //创建事件字段
    size_t dim[2]={1,1};
    dim[0]=events.size();
    matvar_t *event_field=Mat_VarCreateStruct("event",2,dim,event_item_field,8);
    for(size_t i=0;i<dim[0];i++)
    {
        //获取单个事件
        matvar_t *event=events[i];
        for(int j=0;j<8;j++)
        {
            //获取单个事件的字段值
            matvar_t *event_item=Mat_VarGetStructFieldByName(event,event_item_field[j],0);
            if(event_item!=NULL&&event_item->dims[0]!=0)
            {
                size_t field_dim[2]={1,1};
                if(event_item->class_type==MAT_C_CHAR)
                {
                   field_dim[1]=event_item->nbytes;
                }
                char *field_data=(char*)(event_item->data);
                matvar_t *field=Mat_VarCreate(NULL,event_item->class_type,event_item->data_type,2,field_dim,field_data,0);
                Mat_VarSetStructFieldByName(event_field,event_item_field[j],i,field);
            }
        }
    }
    //将事件字段写入eeg字段
    Mat_VarSetStructFieldByName(eeg,"event",0,event_field);
}

void MatRead::writeUrevent()
{
    const char *urevent_item_field[7]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code"
    };
    //创建事件字段
    size_t dim[2]={1,1};
    dim[0]=urevents.size();
    matvar_t *urevent_field=Mat_VarCreateStruct("event",2,dim,urevent_item_field,7);
    for(size_t i=0;i<dim[0];i++)
    {
        //获取单个事件
        matvar_t *event=urevents[i];
        for(int j=0;j<7;j++)
        {
            //获取单个事件的字段值
            matvar_t *event_item=Mat_VarGetStructFieldByName(event,urevent_item_field[j],0);
            if(event_item!=NULL&&event_item->dims[0]!=0)
            {
                size_t field_dim[2]={1,1};
                if(event_item->class_type==MAT_C_CHAR)
                {
                   field_dim[1]=event_item->nbytes;
                }
                char *field_data=(char*)(event_item->data);
                matvar_t *field=Mat_VarCreate(NULL,event_item->class_type,event_item->data_type,2,field_dim,field_data,0);
                Mat_VarSetStructFieldByName(urevent_field,urevent_item_field[j],i,field);
            }
        }
    }
    //将原始事件字段写入eeg字段
    Mat_VarSetStructFieldByName(eeg,"urevent",0,urevent_field);
}




