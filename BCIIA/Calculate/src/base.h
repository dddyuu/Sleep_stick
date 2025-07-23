#ifndef DEEPLEARN_H
#define DEEPLEARN_H

#include "calculate.h"
#include "onnxruntime_cxx_api.h"
#include "iostream"

class Calculate::DeepLearn::Base
{
public:
    Base();
    ~Base();
    void setInputSize(std::vector<int64_t>);
    void setInputNames(std::vector<const char *>);
    void setOuputSize(std::vector<int64_t>);
    void setOuputNames(std::vector<const char *>);
    bool load(wchar_t*);
    bool run(std::vector<float>);
    std::vector<std::vector<float>> getOutputValue();

private:
    Ort::Env env;
    Ort::Session *model;

    std::vector<int64_t> input_size;
    std::vector<const char *> input_names;
    std::vector<int64_t> output_size;
    std::vector<const char *> output_names;
    std::vector<Ort::Value> output_value;


};

#endif // DEEPLEARN_H
