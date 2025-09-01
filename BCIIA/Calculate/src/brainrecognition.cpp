#include "brainrecognition.h"
#include "fstream"
#include "filesystem"
#include "sstream"
#include "exception"
using namespace Calculate::DeepLearn;
BrainRecognition::BrainRecognition()
{
    running=false;
    input_data.resize(32);
    account=0;
    time_duration=0;
    std::vector<int64_t> input_size={1,32,1000};
    setInputSize(input_size);
    std::vector<const char *> input_names={"input"};
    setInputNames(input_names);
    std::vector<const char *> output_names={"output","x.3"};
    setOuputNames(output_names);

    load(L"E:\\learn\\fatigue\\fatigue_merge10s\\BCIIA\\Calculate\\model\\deeplearn\\BrainRecognition.onnx");
   
    loadDatabase();
}
BrainRecognition::~BrainRecognition()
{

}
bool BrainRecognition::append(std::vector<float> data)
{
    if(running)
    {
        if(time_duration==5)
        {
            return false;
        }
        for(size_t i=0;i<data.size();i++)
        {
            input_data[i].push_back(data[i]);
        }
        account++;
        if(account==1000)
        {
           for(size_t i=0;i<input_data.size();i++)
           {
               input_value.insert(input_value.end(),input_data[i].begin(),input_data[i].end());
               input_data[i].clear();
           }
           account=0;
           time_duration++;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool BrainRecognition::readyData()
{
    if(time_duration==5)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int64_t BrainRecognition::run()
{
    return run(input_value);
}
int64_t BrainRecognition::run(std::vector<float> value)
{
    value=normlize(value);
    std::vector<BrainRecognition::MatchResult> match_results;
    for(int i=0;i<5;i++)
    {
        std::vector<float> input_value=std::vector<float>(value.begin()+i*32*1000,value.begin()+(i+1)*32*1000);
        Base::run(input_value);
        std::vector<std::vector<float>> output_value=getOutputValue();
        std::vector<BrainRecognition::MatchResult> match_result=match(output_value[1]);
        match_results.insert(match_results.end(),match_result.begin(),match_result.end());
    }
    time_duration=0;
    input_value.clear();
    running=false;
    return compute_identity(match_results);
}

void BrainRecognition::start()
{
    running=true;
}
float BrainRecognition::cosine_similarity(std::vector<float> vector1, std::vector<float> vector2)
{
    int64_t vector_len=vector1.size();
    float dot_product=0;
    for(int i=0;i<vector_len;i++)
    {
        dot_product+=vector1[i]*vector2[i];
    }
    float norm_vector1=0;
    float norm_vector2=0;
    for(int i=0;i<vector_len;i++)
    {
        norm_vector1+=vector1[i]*vector1[i];
        norm_vector2+=vector2[i]*vector2[i];
    }
    norm_vector1=sqrt(norm_vector1);
    norm_vector2=sqrt(norm_vector2);

    return dot_product/(norm_vector1*norm_vector2);
}

std::vector<BrainRecognition::MatchResult> BrainRecognition::match(std::vector<float> vector)
{
    std::vector<MatchResult> match_results;
    for(auto identity_data:identity_database)
    {
        std::vector<std::vector<float>> feature=identity_data.feature;
        for(auto feature_value:feature)
        {
            MatchResult match_result;
            match_result.id=identity_data.id;
            float score=cosine_similarity(vector,feature_value);
            if(score>0.9)
            {
                match_result.score=score;
                match_results.push_back(match_result);
            }
        }
    }
    std::sort(match_results.begin(),match_results.end());
    if(match_results.size()<30)
    {
        return match_results;
    }
    else
    {
        return  std::vector<MatchResult>(match_results.begin(),match_results.begin()+30);
    }

}

int64_t BrainRecognition::compute_identity(std::vector<BrainRecognition::MatchResult> match_results)
{

    std::vector<BrainRecognition::MatchResult> _match_results=match_results;
    auto last = std::unique(_match_results.begin(), _match_results.end());
    _match_results.erase(last,_match_results.end());
    std::vector<IdentityResult> identity_results;
    for(auto _match_result:_match_results)
    {
       IdentityResult identity_result;
       int64_t id=_match_result.id;
       float max_score=0;
       int64_t count=0;
       for(auto match_result:match_results)
       {
           if(id==match_result.id)
           {
               if(max_score<match_result.score)
               {
                   max_score=match_result.score;
               }
               count++;
           }

       }
       identity_result.id=id;
       identity_result.count=count;
       identity_result.max_score=max_score;
       identity_results.push_back(identity_result);
    }
    if(identity_results.empty())
    {
        return -1;
    }
    if(identity_results.size()==1)
    {
        return identity_results[0].id;
    }
    IdentityResult init_result=identity_results.at(0);
    identity_results.erase(identity_results.begin());
    int64_t identity_id=init_result.id;
    float score=init_result.max_score;
    int64_t count=init_result.count;
    for(auto result:identity_results)
    {
        if(count<result.count)
        {
            count=result.count;
            identity_id=result.id;
            score=result.max_score;
        }
        else if(count==result.count)
            if(score<result.max_score)
            {
                count=result.count;
                identity_id=result.id;
                score=result.max_score;
            }
    }
    if(count<30)
    {
        std::cout << "id:" << identity_id;
        std::cout.flush();
        return -1;
    }
    else
    {
        return identity_id;
    }
    return 0;
}

std::vector<float> BrainRecognition::normlize(std::vector<float> value)
{
    for(size_t i=0;i<value.size();i++)
    {
        if(value[i]>50)
        {
            value[i]=50;
        }
        if(value[i]<-50)
        {
            value[i]=-50;
        }
        value[i]=(value[i]+50)/100;
    }
    return value;

}
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(str);

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}
bool BrainRecognition::loadDatabase()
{
    std::string path="feature/BrainRecognition";
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string feature_path=entry.path().string();
        std::string feature_name=entry.path().filename().string();
        std::string base_filename = splitString(feature_name, '.')[0];
        std::vector<std::string> info = splitString(base_filename, '_');
        std::string name=info[0];
        int64_t id=info[1][0]-48;
        IdentityData identity_data;
        identity_data.id=id;
        identity_data.name=name;
        std::ifstream inputFile(feature_path, std::ios::binary);

        if (!inputFile) {
           std::cerr << "Unable to open file example.bin" << std::endl;
           return 1;
        }
        std::vector<float> buffer(512);
        while (inputFile.read((char*)buffer.data(), 512*sizeof(float)))
        {
           identity_data.feature.push_back(buffer);
        }
        inputFile.close();
        identity_database.push_back(identity_data);
    }
//    std::cout<<identity_database[1].feature.size();
//    std::cout<<identity_database[1].name;
//    std::cout<<identity_database[1].id<<std::endl;
    return false;
}
