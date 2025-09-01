#include "biteteethrecognition.h"
#include "iostream"
using namespace Calculate;
BiteTeethRecognition::BiteTeethRecognition(unsigned char channel_num)
{
    this->channel_num=channel_num;
    baseline.resize(channel_num,0);
    init_baseline_sum.resize(channel_num,0);
    eeg_cache.resize(channel_num);
    init_baseline_num=0;
    threshold_status=false;
    recognition_status=false;
    running=false;
    recognition_mode=false;
    bite_teeth_threshold=60;

    cache_num=0;
    real_cache_num=0;
    peek_count=3;
    valid_channel_num=7;
}

void BiteTeethRecognition::start()
{
    running=true;
}
void BiteTeethRecognition::append(std::vector<float> value)
{
    if(running)
    {
        if(threshold_status&&!recognition_status)
        {
            if(recognition_mode)
            {
                eeg_cache.insert(eeg_cache.end(),value.begin(),value.end());
                cache_num++;
                if(cache_num==3000)
                {
                    recognition_status=true;
                    cache_num=0;
                }
            }
            else
            {
                real_cache.insert(real_cache.end(),value.begin(),value.end());
                real_cache_num++;
                if(real_cache_num==500)
                {
                    recognition_status=true;
                    real_cache_num=0;
                }
            }

        }
    }
}

bool BiteTeethRecognition::recogntion( std::vector<std::vector<float>> value)
{
    unsigned int valid_channel_num=0;
    for(int i=0;i<channel_num;i++)
    {
        std::vector<float> channel_value=value[i];
        float threshold=baseline[i]+bite_teeth_threshold;
        float last_dx=0;
        float this_dx=0;
        unsigned int peek_count=0;
        for(size_t j=1;j<channel_value.size();j++)
        {
            if(channel_value[j]>threshold)
            {
                 this_dx=channel_value[j]-channel_value[j-1];
                 if(last_dx==0)
                 {
                     last_dx=this_dx;
                 }
                 bool result=isPeek(this_dx,last_dx);
                 if(result)
                 {
                    peek_count++;
                    if(peek_count>this->peek_count)
                    {
                        break;
                    }
                 }
            }
        }
        if(peek_count>this->peek_count)
        {
            valid_channel_num++;
        }
    }
    if(valid_channel_num>this->valid_channel_num)
    {
        std::cout<<"valid_channel_num:"<<valid_channel_num <<std::endl;
    }

    if(valid_channel_num>this->valid_channel_num)
    {
        return true;
    }
    else {
        return false;
    }
}

bool BiteTeethRecognition::recogntion()
{
    std::vector<std::vector<float>> eeg(channel_num);
    for(size_t i=0;i<(real_cache.size()/channel_num);i++)
    {
       for(int j=0;j<channel_num;j++)
       {
           eeg[j].push_back(real_cache[i*32+j]);
       }
    }
    real_cache.clear();
    recognition_status=false;
    return recogntion(eeg);
}
unsigned int BiteTeethRecognition::recogntionCommand()
{

    std::vector<std::vector<float>> eeg1(channel_num);
    std::vector<std::vector<float>> eeg2(channel_num);
    std::vector<std::vector<float>> eeg3(channel_num);
    for(int i=0;i<1000;i++)
    {
       for(int j=0;j<32;j++)
       {
           eeg1[j].push_back(eeg_cache[i*32+j]);
           eeg2[j].push_back(eeg_cache[i*32+j+32*1000]);
           eeg3[j].push_back(eeg_cache[i*32+j+32*2000]);

       }
    }
    unsigned int result1=recogntion(eeg1);
    unsigned int result2=recogntion(eeg2);
    unsigned int result3=recogntion(eeg3);
    eeg_cache.clear();
    recognition_status=false;
    if(result1==1&&result2==1&&result3==1)
    {
        return 4;
    }
    return (result1<<1)+result3;
}

bool BiteTeethRecognition::recognitionStatus()
{
    return recognition_status;
}

bool BiteTeethRecognition::recognitionModel()
{
    return recognition_mode;
}

void BiteTeethRecognition::setRecognitionModel(bool model)
{
    recognition_mode=model;
    eeg_cache.clear();
    real_cache.clear();
}

void BiteTeethRecognition::initBiteTeethThreshold(std::vector<float> value)
{
    if(running)
    {
        if(!threshold_status)
        {
            for(size_t i=0;i<value.size();i++)
            {
                init_baseline_sum[i]+=value[i];
            }
            init_baseline_num++;
            if(init_baseline_num==5000)
            {
                for(size_t i=0;i<baseline.size();i++)
                {
                    baseline[i]=init_baseline_sum[i]/init_baseline_num;
                }
                threshold_status=true;
            }
        }
    }

}

bool BiteTeethRecognition::isPeek(float this_dx,float last_dx)
{
    float diff=this_dx-last_dx;
    if(diff>0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
