#ifndef BITETEETHRECOGNITION_H
#define BITETEETHRECOGNITION_H

#include "calculate.h"
#include "vector"
class Calculate::BiteTeethRecognition
{
public:
    BiteTeethRecognition(unsigned char channel_num);
    //开始识别
    void start();
    //插入eeg信号
    void append(std::vector<float>);
    //是否咬牙
    bool recogntion(std::vector<std::vector<float>> value);
    bool recogntion();
    //识别控制指令
    unsigned int recogntionCommand();

    //是否识别指令
    bool recognitionStatus();

    bool recognitionModel();
    void setRecognitionModel(bool);
    void initBiteTeethThreshold(std::vector<float>);


private:
    //eeg信号基线
    std::vector<float> baseline;
    //咬牙阈值
    float bite_teeth_threshold;
    //初始化基线的采样点之和
    std::vector<float> init_baseline_sum;
    //初始化基线的采样点数目
    unsigned int init_baseline_num;
    //是否开始运行
    bool running;
    //阈值是否初始化
    bool threshold_status;
    //是否识别
    bool recognition_status;
    //电极通道数
    unsigned char channel_num;

    bool recognition_mode;//ture:批量，false:实时

    //是否存在峰值或者峰谷
    bool isPeek(float,float);

    //批量识别
    //eeg信号缓存
    std::vector<float> eeg_cache;
    //缓存数目
    unsigned int cache_num;


    //实时识别
    std::vector<float> real_cache;
    unsigned int real_cache_num;

    //咬牙的有效判断
    //峰值数目
    unsigned int peek_count;
    //通道有效
    unsigned int valid_channel_num;




};

#endif // BITETEETHRECOGNITION_H
