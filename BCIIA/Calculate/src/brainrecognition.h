#ifndef BRAINRECOGNITION_H
#define BRAINRECOGNITION_H

#include "calculate.h"
#include "base.h"
class Calculate::DeepLearn::BrainRecognition:public Calculate::DeepLearn::Base
{
public:
    BrainRecognition();
    ~BrainRecognition();
    bool append(std::vector<float>);
    bool readyData();
    int64_t run();
    int64_t run(std::vector<float>);
    void start();
private:
    typedef struct
    {
       std::string name;
       int64_t id;
       std::vector<std::vector<float>> feature;
    }IdentityData;
    typedef struct MatchResult
    {
        int64_t id;
        float score;
        bool operator<(const MatchResult& other)const{
            return score<other.score;
        }
        bool operator==(const MatchResult& other)const{
            return id==other.id;
        }
    }MatchResult;
    typedef struct
    {
       int64_t count;
       int64_t id;
       float max_score;
    }IdentityResult;
    float cosine_similarity(std::vector<float>,std::vector<float>);
    std::vector<MatchResult> match(std::vector<float>);
    int64_t compute_identity(std::vector<MatchResult>);
    std::vector<float> normlize(std::vector<float>);
    //特征加载
    bool loadDatabase();
    std::vector<IdentityData> identity_database;

    //输入数据
    std::vector<std::vector<float>> input_data;
    //模型输入数据
    std::vector<float> input_value;
    int64_t account;
    unsigned int time_duration;

    //算法是否运行
    bool running;
};

#endif // BRAINRECOGNITION_H
