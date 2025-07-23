#include "base.h"
int main()
{
    Calculate::DeepLearn::Base base;
    std::cout<<base.load(L"mnist.onnx")<<std::endl;
    return 0;
}
