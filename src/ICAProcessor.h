#ifndef ICAPROCESSOR_H
#define ICAPROCESSOR_H

#include <vector>
#include <string>
#include <memory>
#include "ICA.h"

class ICAProcessor {
public:
    ICAProcessor();

    // 输入数据类型为 float
    void setData(const std::vector<std::vector<float>>& data);

    // 输入数据类型为 float
    std::pair<std::vector<int>, std::vector<double>>
        calculateICAForEOG(const std::vector<std::string>& ch_name = {},
            double threshold = 0.3);

    // 返回 float 类型数据
    std::vector<std::vector<float>> applyICA();

private:
    std::vector<std::vector<float>> raw_resampled_; // [channels][samples]
    std::unique_ptr<ICA> ica_; // ICA 计算仍然使用 double
};

#endif // ICAPROCESSOR_H