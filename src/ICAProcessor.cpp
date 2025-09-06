#include "ICAProcessor.h"
#include <stdexcept>

ICAProcessor::ICAProcessor() {}

// 修改输入数据类型为 float
void ICAProcessor::setData(const std::vector<std::vector<float>>& data) {
    if (data.empty()) throw std::runtime_error("Data is empty.");
    const size_t n_samples = data[0].size();
    for (const auto& ch : data) {
        if (ch.size() != n_samples) {
            throw std::runtime_error("All channels must have the same number of samples.");
        }
    }
    raw_resampled_ = data;
    ica_.reset();
}

// 修改输入数据类型为 float
std::pair<std::vector<int>, std::vector<double>>
ICAProcessor::calculateICAForEOG(const std::vector<std::string>& ch_name,
    double threshold) {
    if (raw_resampled_.empty()) {
        throw std::runtime_error("Data is not set.");
    }

    const int n_channels = static_cast<int>(raw_resampled_.size());
    const int n_samples = static_cast<int>(raw_resampled_[0].size());

    // 将 float 数据转换为 Eigen::MatrixXd (double)
    Eigen::MatrixXd data(n_channels, n_samples);
    for (int c = 0; c < n_channels; ++c)
        for (int t = 0; t < n_samples; ++t)
            data(c, t) = static_cast<double>(raw_resampled_[c][t]);

    if (!ica_) ica_ = std::make_unique<ICA>(n_channels, /*random_state=*/97);
    ica_->fit(data);

    Eigen::MatrixXd eog_ref;
    if (!ch_name.empty()) {
        std::vector<int> ref_idx;
        ref_idx.reserve(ch_name.size());
        for (int i = 0; i < static_cast<int>(ch_name.size()) && i < n_channels; ++i) {
            if (!ch_name[i].empty()) ref_idx.push_back(i);
        }
        if (ref_idx.empty()) {
            throw std::runtime_error("No valid EOG reference channels found from ch_name.");
        }
        eog_ref.resize(static_cast<int>(ref_idx.size()), n_samples);
        for (int r = 0; r < static_cast<int>(ref_idx.size()); ++r) {
            eog_ref.row(r) = data.row(ref_idx[r]);
        }
    }
    else {
        eog_ref = data;
    }

    auto res = ica_->find_bads_eog(data, eog_ref, threshold);
    std::vector<int> bad_idx = res.first;
    std::vector<double> scores = res.second;

    ica_->set_exclude(bad_idx);
    return std::make_pair(bad_idx, scores);
}

// 返回 float 类型数据
std::vector<std::vector<float>> ICAProcessor::applyICA() {
    if (raw_resampled_.empty()) {
        throw std::runtime_error("Data is not set.");
    }
    const int n_channels = static_cast<int>(raw_resampled_.size());
    const int n_samples = static_cast<int>(raw_resampled_[0].size());

    // 将 float 数据转换为 Eigen::MatrixXd (double)
    Eigen::MatrixXd data(n_channels, n_samples);
    for (int c = 0; c < n_channels; ++c)
        for (int t = 0; t < n_samples; ++t)
            data(c, t) = static_cast<double>(raw_resampled_[c][t]);

    if (!ica_) {
        ica_ = std::make_unique<ICA>(n_channels, /*random_state=*/97);
        ica_->fit(data);
    }

    Eigen::MatrixXd cleaned = ica_->apply(data);

    // 将 Eigen::MatrixXd (double) 转换回 float
    std::vector<std::vector<float>> out(n_channels, std::vector<float>(n_samples, 0.0f));
    for (int c = 0; c < n_channels; ++c)
        for (int t = 0; t < n_samples; ++t)
            out[c][t] = static_cast<float>(cleaned(c, t));

    return out;
}