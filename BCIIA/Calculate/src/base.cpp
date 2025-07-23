#include "base.h"
using namespace Calculate::DeepLearn;
Base::Base()
{
    model=NULL;
}

Base::~Base()
{
    delete model;
}

void Base::setInputSize(std::vector<int64_t> size)
{
    this->input_size=size;
}

void Base::setInputNames(std::vector<const char *> input_names)
{
    this->input_names=input_names;
}

void Base::setOuputSize(std::vector<int64_t> size)
{
    this->output_size=size;
}

void Base::setOuputNames(std::vector<const char *> output_names)
{
    this->output_names=output_names;
}
bool Base::load(wchar_t * name)
{
    try {
        model=new Ort::Session(env,name, Ort::SessionOptions{nullptr});
        
        //获取输入名字
//        std::vector<const char*> input_name;
//        size_t num_input_nodes = model->GetInputCount();
//        Ort::AllocatorWithDefaultOptions allocator;
//        for (size_t i = 0; i < num_input_nodes; i++) {
//            Ort::AllocatedStringPtr name = model->GetInputNameAllocated(i, allocator);
//            input_name.push_back(name.get());
//        }
//        setInputNames(input_name);
//        // 获取输出名字
//        std::vector<const char*> output_name;
//        size_t num_output_nodes = model->GetOutputCount();
//        std::vector<const char*> output_names(num_output_nodes);
//        for (size_t i = 0; i < num_output_nodes; i++) {
//            Ort::AllocatedStringPtr name = model->GetOutputNameAllocated(i, allocator);
//            output_name.push_back(name.get());
//        }
//        setOuputNames(output_name);
        //std::cout<<model->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape().at(1);
        return true;
    } catch (const Ort::Exception& exception) {
        std::cout<<"model load error";
        return false;
    }

}

bool Base::run(std::vector<float> input_value)
{
    if(input_value.empty())
    {

       throw "input is empty";

    }
    if(model==NULL)
    {
        throw "the model is not load";
    }
    if(input_size.empty())
    {
       throw "input_size is not setted";
    }
    // 初始化 ONNX Runtime 环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    auto memory_info=Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator,OrtMemTypeCPU);
    //std::cout<<"memery normal";
    Ort::Value input_tensor=Ort::Value::CreateTensor(memory_info,input_value.data(),input_value.size(),
                                                     input_size.data(),input_size.size());

    Ort::RunOptions run_options;
    try {
      output_value=model->Run(run_options, input_names.data(), &input_tensor, input_names.size(), output_names.data(),output_names.size());
      return true;
    } catch (const Ort::Exception& exception) {
       std::cout<<exception.what();
      return false;
    }
}

std::vector<std::vector<float>> Base::getOutputValue()
{
   std::vector<std::vector<float>> output_value;
   for(size_t i=0;i<this->output_value.size();i++)
   {
       std::vector<float> value;
       std::vector<int64_t> output_shape=this->output_value[i].GetTensorTypeAndShapeInfo().GetShape();
       int64_t data_len=1;
       for(auto shape_value:output_shape)
       {
           data_len*=shape_value;
       }
       value.resize(data_len);
       memcpy(value.data(),this->output_value[i].GetTensorMutableData<float>(),data_len*sizeof (float));
       output_value.push_back(value);
   }
   return output_value;
}
