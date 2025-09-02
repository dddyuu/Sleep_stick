#ifndef EEGPROCESSOR_H
#define EEGPROCESSOR_H

#include <QList>
#include <QObject>
#include <vector>
#include <cmath>
#include <stdexcept>
#include<qdebug.h>
class EEGProcessor : public QObject {
    Q_OBJECT
public:
    EEGProcessor(double original_sfreq, double target_sfreq,
        size_t num_channels, int filter_order = 101);

    void setSize(size_t num_samples);  // 设置需要的样本数
    void append(const QList<double>& data);  // 添加数据
    bool isReady() const;  // 检查是否收集了足够数据
    std::vector<std::vector<float>> getOutput();  // 获取处理后的数据

private:
    // 内部处理函数
    void designBandpassFIR();
    QList<double> applyFIR(const QList<double>& channel_data, size_t channel_idx);
    QList<double> resampleChannel(const QList<double>& data);

    // 处理完整数据
    QList<QList<double>> process(const QList<double>& input_data, size_t num_samples);

    // 参数
    double original_sfreq_, target_sfreq_;
    size_t num_channels_;
    int filter_order_;

    // FIR滤波器系数
    QList<double> fir_coeffs_;
    QList<QList<double>> filter_buffers_;

    // 数据收集相关
    QList<double> data_buffer_;
    size_t count_ = 0;
    size_t required_samples_ = 0;
    std::vector<std::vector<float>> output_buffer_;
};

#endif // EEGPROCESSOR_H