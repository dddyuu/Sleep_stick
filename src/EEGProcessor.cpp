#include "EEGProcessor.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EEGProcessor::EEGProcessor(double original_sfreq, double target_sfreq,
    size_t num_channels, int filter_order)
    : original_sfreq_(original_sfreq),
    target_sfreq_(target_sfreq),
    num_channels_(num_channels),
    filter_order_(filter_order) {

    if (original_sfreq <= target_sfreq) {
        throw std::invalid_argument("Ŀ������ʱ���С��ԭʼ������");
    }

    designBandpassFIR();

    // ��ʼ���˲���״̬
    filter_buffers_.clear();
    for (size_t i = 0; i < num_channels_; i++) {
        QList<double> buf;
        for (int j = 0; j < filter_order_; j++) {
            buf.append(0.0);
        }
        filter_buffers_.append(buf);
    }
}

void EEGProcessor::setSize(size_t num_samples) {
    required_samples_ = num_samples * num_channels_;
    data_buffer_.clear();
    data_buffer_.reserve(required_samples_);
    count_ = 0;
}

void EEGProcessor::append(const QList<double>& data) {
    for (const auto& val : data) {
        if (count_ < required_samples_) {
            data_buffer_.append(val);
            count_++;
        }
    }

    if (isReady()) {
        auto processed = process(data_buffer_, required_samples_ / num_channels_);

        // ת��Ϊvector<vector<float>>��ʽ
        output_buffer_.clear();
        for (const auto& channel : processed) {
            std::vector<float> channel_data;
            channel_data.reserve(channel.size());
            for (const auto& val : channel) {
                channel_data.push_back(static_cast<float>(val));
            }
            output_buffer_.push_back(channel_data);
        }

        // ���û�����
        data_buffer_.clear();
        count_ = 0;
    }
}

bool EEGProcessor::isReady() const {
    return count_ >= required_samples_;
}

std::vector<std::vector<float>> EEGProcessor::getOutput() {
    auto output = output_buffer_;
    output_buffer_.clear();
    return output;
}

// ��ͨ�˲������
void EEGProcessor::designBandpassFIR() {
    fir_coeffs_.clear();
    const int M = filter_order_ - 1;
    const double f1 = 0.5 / (original_sfreq_ / 2.0);
    const double f2 = 45.0 / (original_sfreq_ / 2.0);

    for (int n = 0; n < filter_order_; n++) {
        double coeff = 0.0;
        if (n == M / 2) {
            coeff = 2.0 * (f2 - f1);
        }
        else {
            double h = (sin(2 * M_PI * f2 * (n - M / 2.0)) -
                sin(2 * M_PI * f1 * (n - M / 2.0))) /
                (M_PI * (n - M / 2.0));
            double w = 0.54 - 0.46 * cos(2 * M_PI * n / M);
            coeff = h * w;
        }
        fir_coeffs_.append(coeff);
    }
}

QList<double> EEGProcessor::applyFIR(const QList<double>& data, size_t ch_idx) {
    if (ch_idx >= static_cast<size_t>(filter_buffers_.size())) {
        throw std::out_of_range("ͨ������������Χ");
    }

    QList<double> output;
    output.reserve(data.size());
    auto& buf = filter_buffers_[static_cast<int>(ch_idx)];

    for (int i = 0; i < data.size(); i++) {
        for (int j = filter_order_ - 1; j > 0; j--) {
            buf[j] = buf[j - 1];
        }
        buf[0] = data[i];

        double sum = 0.0;
        for (int j = 0; j < filter_order_; j++) {
            sum += buf[j] * fir_coeffs_[j];
        }
        output.append(sum);
    }
    return output;
}
// 通道降采样
QList<double> EEGProcessor::resampleChannel(const QList<double>& data) {
    const double ratio = original_sfreq_ / target_sfreq_;
    const int out_len = static_cast<int>(data.size() / ratio);
    QList<double> output;
    output.reserve(out_len);
    
    // 添加抗混叠滤波器：如果降采样比例大于2，需要更强的滤波
    QList<double> filtered_data = data;
    if (ratio > 2.0) {
        // 简单的移动平均滤波作为抗混叠
        QList<double> temp = filtered_data;
        for (int i = 1; i < temp.size() - 1; i++) {
            filtered_data[i] = (temp[i-1] + temp[i] + temp[i+1]) / 3.0;
        }
    }

    for (int i = 0; i < out_len; i++) {
        double pos = i * ratio;
        int idx1 = static_cast<int>(pos);
        int idx2 = (idx1 + 1 < filtered_data.size()) ? idx1 + 1 : idx1;
        
        if (idx1 >= filtered_data.size()) {
            idx1 = filtered_data.size() - 1;
            idx2 = idx1;
        }
        
        double frac = pos - idx1;
        
        // 线性插值
        double interpolated = filtered_data[idx1] * (1.0 - frac) + filtered_data[idx2] * frac;
        
        // 避免数值不稳定
        if (std::isnan(interpolated) || std::isinf(interpolated)) {
            interpolated = 0.0;
        }
        
        output.append(interpolated);
    }
    
    qDebug() << "Resampled from " << data.size() << " to " << output.size() << " (ratio:" << ratio << ")";
    return output;
}

QList<QList<double>> EEGProcessor::process(const QList<double>& input, size_t num_samples) {
    QList<QList<double>> result;

    if (input.size() != static_cast<int>(num_channels_ * num_samples)) {
        throw std::invalid_argument("�������ݳߴ���ͨ������ƥ��");
    }
	qDebug() << input.size()<<" \n"<<"num_samples: "<<  num_samples;

    for (size_t ch = 0; ch < num_channels_; ch++) {
        QList<double> ch_data;
        ch_data.reserve(static_cast<int>(num_samples));

        int start_index = static_cast<int>(ch * num_samples);
        for (int i = 0; i < static_cast<int>(num_samples); i++) {
            ch_data.append(input[start_index + i]);
        }

        auto filtered = applyFIR(ch_data, ch);
        result.append(resampleChannel(filtered));
    }
    return result;
}