#include "tskfatigue.h"
#include <onnxruntime_cxx_api.h>

using namespace Calculate;
TskFatigue::TskFatigue()
{
    //std::cout<<"sucess";
    std::vector<int64_t> input_size = {1,25};
    setInputSize(input_size);
    std::vector<const char*> input_names = { "input_name" }; // 分类“float_input”
    setInputNames(input_names);
    std::vector<const char*> output_names = { "output_name"};//分类输出“output_name”
    setOuputNames(output_names);
    load(L"D:/Qt_Projects/Sleep_stick/BCIIA/Calculate/model/deeplearn/four_model.onnx");
    std::cout.flush();

}

TskFatigue::~TskFatigue()
{

}
