#include "mat.h"
#include "QDebug"
MAT::MAT(QObject *parent):QObject(parent)
{
}

MAT::~MAT()
{
}

void MAT::setFileName(QString name)
{
    qDebug() << name;
    mat_fp=Mat_CreateVer(name.toStdString().c_str(),NULL,MAT_FT_DEFAULT);
}
void MAT::start(QStringList signals_name)
{
    size_t dim[2] = {1,1};
    const char* field[12] = { "chanlocs","srate","filename","event","urevent","ref","age","set","eeg","label"};
    //for (int i = 0; i < signals_name.size(); i++)
    //{
    //    qDebug() << signals_name[i].size();
    //    char _field[10] = {0};
    //    for (int j = 0; j < signals_name[i].size(); j++)
    //    {
    //        _field[j] = signals_name[i].toStdString()[j];
    //    }
    //    field[i + 8] = _field;
    //}
    eeg_struct = Mat_VarCreateStruct("EEG", 2, dim, field, 9+ signals_name.size());
    qDebug() << 123;
}
void MAT::save(QList<double*> data, QList<quint8> label, QList<quint64> data_len,QList<quint8> channel_num,QStringList signals_name)
{
    for (int i = 0; i < signals_name.size(); i++)
    {
        double* Matdata = data[i];
        QList<quint8> labelsave = label;
        size_t dims[2];
        dims[0] = channel_num[i];
        dims[1] = data_len[i]/channel_num[i];
        qDebug() << dims[0] << " " << dims[1];
        matvar_t* data_field = Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, Matdata, 0);
        Mat_VarSetStructFieldByName(eeg_struct,signals_name[i].toStdString().c_str(), 0, data_field);
    }
    // 2. 保存所有标签值（作为一个整体）
    if (!label.isEmpty()) {
        size_t label_dims[2] = {1, static_cast<size_t>(label.size())}; // 行向量

        // 复制标签数据到新数组
        quint8* label_data = new quint8[label.size()];
        for (int i = 0; i < label.size(); i++) {
            label_data[i] = label[i];
        }

        // 创建MAT变量
        matvar_t* label_field = Mat_VarCreate(
            NULL,
            MAT_C_UINT8,
            MAT_T_UINT8,
            2,
            label_dims,
            label_data,
            0
            );

        // 添加到结构体
        Mat_VarSetStructFieldByName(
            eeg_struct,
            "label",  // 使用固定字段名
            0,
            label_field
            );

        // 释放临时数组
        delete[] label_data;
    }
}
void MAT::stop()
{
    //数据写入mat文件
    Mat_VarWrite(mat_fp,eeg_struct,MAT_COMPRESSION_NONE);
    //释放mat数据的空间
    Mat_VarFree(eeg_struct);
    eeg_struct=NULL;
    //关闭mat文件指针
    Mat_Close(mat_fp);
    mat_fp=NULL;
}
void MAT::saveChanlocs(QVariantList chanlocs)
{

    const char *chanlocs_item_field[12]={
        "labels","type","theta","radius",
        "x","y","z","sph_theta","sph_phi",
        "sph_radius","urchan","ref"
    };
    size_t dim[2]={1,1};
    dim[0]=chanlocs.size();
    matvar_t *chanlocs_field=Mat_VarCreateStruct("chanlocs",2,dim,chanlocs_item_field,12);
    int index=0;
    for(QVariantList::const_iterator begin=chanlocs.begin();begin!=chanlocs.end();begin++)
    {
        QVariantMap map=(*begin).value<QVariantMap>();
        quint8 map_size=map.size();
        QStringList keys=map.keys();
        for(int i=0;i<map_size;i++)
        {
            QString key=keys[i];
            QVariant value=map[key];
            if(value.type()==QVariant::String)
            {
                QString data=value.value<QString>();
                int len=data.size();
                size_t dim[2];
                dim[0]=1;
                dim[1]=len;
                matvar_t *filename_field=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dim,data.toUtf8().data(),0);
                Mat_VarSetStructFieldByName(chanlocs_field,key.toUtf8().data(),index,filename_field);
            }
            else if(value.type()==QVariant::Double)
            {
                size_t dim[2]={1,1};
                double data=value.value<double>();
                matvar_t *field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dim,&data,0);
                Mat_VarSetStructFieldByName(chanlocs_field,key.toUtf8().data(),index,field);
            }
            else if(value.type()==QVariant::Int)
            {
                size_t dim[2]={1,1};
                int data=value.value<int>();
                matvar_t *field = Mat_VarCreate(NULL,MAT_C_UINT32,MAT_T_UINT32,2,dim,&data,0);
                Mat_VarSetStructFieldByName(chanlocs_field,key.toUtf8().data(),index,field);
            }


        }
        index++;
    }
    Mat_VarSetStructFieldByName(eeg_struct,"chanlocs",0,chanlocs_field);
}
void MAT::saveFileName(QString name)
{
   QFileInfo info(name);
   name=info.fileName();
   int len=name.size();
   size_t dim[2];
   dim[0]=1;
   dim[1]=len;
   matvar_t *filename_field=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dim,name.toUtf8().data(),0);
   Mat_VarSetStructFieldByName(eeg_struct,"filename",0,filename_field);
}

void MAT::saveSrate(quint16 rate)
{
    size_t value=rate;
    size_t dim[2]={1,1};
    matvar_t *srate_field=Mat_VarCreate(NULL,MAT_C_UINT16,MAT_T_UINT16,2,dim,&value,0);
    Mat_VarSetStructFieldByName(eeg_struct,"srate",0,srate_field);
}

void MAT::saveEevent(QVariantList event)
{
    const char *event_item_field[8]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code","urevent"
    };
    size_t dim[2]={1,1};
    dim[0]=event.size();
    matvar_t *event_field=Mat_VarCreateStruct("event",2,dim,event_item_field,8);
    int index=0;
    for(QVariantList::const_iterator begin=event.begin();begin!=event.end();begin++)
    {
        QVariantMap map=(*begin).value<QVariantMap>();
        quint8 map_size=map.size();
        QStringList keys=map.keys();
        for(int i=0;i<map_size;i++)
        {
            QString key=keys[i];
            QVariant value=map[key];
            if(value.type()==QVariant::String)
            {
                QString data=value.value<QString>();
                int len=data.size();
                size_t dim[2];
                dim[0]=1;
                dim[1]=len;
                matvar_t *filename_field=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dim,data.toUtf8().data(),0);
                Mat_VarSetStructFieldByName(event_field,key.toUtf8().data(),index,filename_field);
            }
            else if(value.type()==QVariant::Double)
            {
                size_t dim[2]={1,1};
                double data=value.value<double>();
                matvar_t *field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dim,&data,0);
                Mat_VarSetStructFieldByName(event_field,key.toUtf8().data(),index,field);
            }
            else if(value.type()==QVariant::Int)
            {
                size_t dim[2]={1,1};
                int data=value.value<int>();
                 matvar_t *field = Mat_VarCreate(NULL,MAT_C_UINT32,MAT_T_UINT32,2,dim,&data,0);
                Mat_VarSetStructFieldByName(event_field,key.toUtf8().data(),index,field);
            }
        }
        index++;
    }
    Mat_VarSetStructFieldByName(eeg_struct,"event",0,event_field);
}
void MAT::saveUrevent(QVariantList event)
{
    const char *urevent_item_field[7]={
        "latency","duration","channel","bvtime","bvmknum",
        "type","code"
    };
    size_t dim[2]={1,1};
    dim[0]=event.size();
    matvar_t *urevent_field=Mat_VarCreateStruct("event",2,dim,urevent_item_field,7);
    int index=0;
    for(QVariantList::const_iterator begin=event.begin();begin!=event.end();begin++)
    {
        QVariantMap map=(*begin).value<QVariantMap>();
        quint8 map_size=map.size();
        QStringList keys=map.keys();
        for(int i=0;i<map_size;i++)
        {
            QString key=keys[i];
            QVariant value=map[key];
            if(value.type()==QVariant::String)
            {
                QString data=value.value<QString>();
                int len=data.size();
                size_t dim[2];
                dim[0]=1;
                dim[1]=len;
                matvar_t *filename_field=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,dim,data.toUtf8().data(),0);
                Mat_VarSetStructFieldByName(urevent_field,key.toUtf8().data(),index,filename_field);
            }
            else if(value.type()==QVariant::Double)
            {
                size_t dim[2]={1,1};
                double data=value.value<double>();
                matvar_t *field = Mat_VarCreate(NULL,MAT_C_DOUBLE,MAT_T_DOUBLE,2,dim,&data,0);
                Mat_VarSetStructFieldByName(urevent_field,key.toUtf8().data(),index,field);
            }
            else if(value.type()==QVariant::Int)
            {
                size_t dim[2]={1,1};
                int data=value.value<int>();
                matvar_t *field = Mat_VarCreate(NULL,MAT_C_UINT32,MAT_T_UINT32,2,dim,&data,0);
                Mat_VarSetStructFieldByName(urevent_field,key.toUtf8().data(),index,field);
            }


        }
        index++;
    }
    Mat_VarSetStructFieldByName(eeg_struct,"urevent",0,urevent_field);
}

void MAT::saveRef(QStringList ref)
{
    size_t dim[2]={0,1};
    int ref_size=ref.size();
    dim[0]=ref_size;
    matvar_t *ref_field=Mat_VarCreate(NULL,MAT_C_CELL,MAT_T_UTF8,2,dim,NULL,0);
    for(int i=0;i<ref_size;i++)
    {
        size_t cell_dim[2];
        cell_dim[0]=1;
        cell_dim[1]=ref[i].size();
        matvar_t *cell=Mat_VarCreate(NULL,MAT_C_CHAR,MAT_T_UTF8,2,cell_dim,ref[i].toUtf8().data(),0);
        Mat_VarSetCell(ref_field,i,cell);
    }
    Mat_VarSetStructFieldByName(eeg_struct,"ref",0,ref_field);
}

void MAT::saveAge(quint8 age)
{
    size_t value=age;
    size_t dim[2]={1,1};
    matvar_t *age_field=Mat_VarCreate(NULL,MAT_C_UINT16,MAT_T_UINT16,2,dim,&value,0);
    Mat_VarSetStructFieldByName(eeg_struct,"age",0,age_field);
}

void MAT::saveSetCode(quint8 code)
{
    size_t value=code;
    size_t dim[2]={1,1};
    matvar_t *set_field=Mat_VarCreate(NULL,MAT_C_UINT16,MAT_T_UINT16,2,dim,&value,0);
    Mat_VarSetStructFieldByName(eeg_struct,"set",0,set_field);
}
