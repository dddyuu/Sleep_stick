#ifndef RECOGNITION_H
#define RECOGNITION_H

#include "base.h"
#include <vector>

namespace Calculate {
    class Recognition;
}

class Calculate::Recognition : public Calculate::DeepLearn::Base
{
public:
    Recognition();
    ~Recognition();

    // 设置输入数据（三维数据展平为一维向量）
    void setInputData(const std::vector<std::vector<std::vector<float>>>& input_3d);

    // 获取分层输出结果
    std::vector<std::vector<float>> getHierarchicalOutput();

private:
    // 三维输入数据缓存 [1][2][250]
    std::vector<std::vector<std::vector<float>>> input_buffer;
};

#endif // RECOGNITION_H