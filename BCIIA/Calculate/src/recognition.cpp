#include "recognition.h"
#include <iostream>
#include <stdexcept>

using namespace Calculate;

Recognition::Recognition()
{
    // 设置三维输入尺寸 [batch=1, channels=2, timesteps=250]
    std::vector<int64_t> input_size = { 1, 2, 250 };
    setInputSize(input_size);

    // ONNX模型输入/输出节点名称（需与导出模型匹配）
    std::vector<const char*> input_names = { "input" };
    setInputNames(input_names);

    std::vector<const char*> output_names = {
        "coarse_out1",
        "fine_out1",
        "coarse_out2",
        "fine_out2",
        "domain_out",
        "feature_out"
    };
    setOuputNames(output_names);

    // 加载预训练模型（路径需根据实际情况修改）
    load(L"E:/learn/sleep/Sleep_stick/BCIIA/Calculate/model/deeplearn/gr.onnx");
    std::cout.flush();
}

Recognition::~Recognition()
{
    // 清理资源
}

void Recognition::setInputData(
    const std::vector<std::vector<std::vector<float>>>& input_3d)
{
    if (input_3d.empty() || input_3d[0].size() != 2 || input_3d[0][0].size() != 250) {
        throw std::invalid_argument("Input must be 3D array [1][2][250]");
    }

    // 深拷贝数据
    input_buffer = input_3d;

    // 展平三维数据为一维向量 (1 * 2 * 250=500)
    std::vector<float> flat_input;
    flat_input.reserve(500);
    for (const auto& channel : input_buffer[0]) {
        flat_input.insert(flat_input.end(), channel.begin(), channel.end());
    }

    // 执行推理
    if (!run(flat_input)) {
        throw std::runtime_error("Model inference failed");
    }
}

std::vector<std::vector<float>> Recognition::getHierarchicalOutput()
{
    // 获取原始输出向量 [6个输出节点]
    auto raw_output = getOutputValue();

    if (raw_output.size() != 6) {
        throw std::runtime_error("Unexpected output structure");
    }

    // 按模型规范组织输出：
    // [0]: coarse_out_1 (1x2)
    // [1]: fine_out_1   (1x2)
    // [2]: coarse_out_2 (1x2)
    // [3]: fine_out_2   (1x2)
    // [4]: domain_out   (1x1)
    // [5]: fine_feat    (1x128)
    return raw_output;
}