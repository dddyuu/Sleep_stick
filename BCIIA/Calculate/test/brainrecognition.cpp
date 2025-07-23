#include "brainrecognition.h"
#include "random"
#include "fstream"
int main()
{
    Calculate::DeepLearn::BrainRecognition brainrecognition;
    std::ifstream inputFile("test.bin", std::ios::binary);

    if (!inputFile) {
       std::cerr << "Unable to open file example.bin" << std::endl;
       return 1;
    }
    std::vector<float> buffer(32*5000);
    inputFile.read((char*)buffer.data(), 32*5000*sizeof(float));
    for(int i=0;i<5000;i++)
    {
        std::vector<float> value=std::vector<float>(buffer.begin()+i*32,buffer.begin()+(i+1)*32);
        brainrecognition.append(value);
    }
    std::cout<<brainrecognition.run();

    std::ifstream inputFile1("test1.bin", std::ios::binary);

    if (!inputFile) {
       std::cerr << "Unable to open file example.bin" << std::endl;
       return 1;
    }
    std::vector<float> buffer1(32*5000);
    inputFile1.read((char*)buffer1.data(), 32*5000*sizeof(float));
    for(int i=0;i<5000;i++)
    {
        std::vector<float> value=std::vector<float>(buffer1.begin()+i*32,buffer1.begin()+(i+1)*32);
        brainrecognition.append(value);
        if(brainrecognition.readyData())
        {
           std::cout<<brainrecognition.run();
        }
    }
    for(int i=0;i<5000;i++)
    {
        std::vector<float> value=std::vector<float>(buffer.begin()+i*32,buffer.begin()+(i+1)*32);
        brainrecognition.append(value);
    }
    std::cout<<brainrecognition.run();
    return 0;
}
