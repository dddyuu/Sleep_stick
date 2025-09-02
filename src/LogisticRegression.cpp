#include "LogisticRegression.h"
#include <cmath>
#include <numeric>
#include <iostream>

LogisticRegression::LogisticRegression(
    std::string penalty,
    double C,
    bool fit_intercept,
    int max_iter,
    double tol,
    double learning_rate
) : penalty_(penalty),
C_(C),
fit_intercept_(fit_intercept),
max_iter_(max_iter),
tol_(tol),
learning_rate_(learning_rate),
intercept_(0.0),
is_standardized_(false) {
}

double LogisticRegression::sigmoid(double z) const {
    return 1.0 / (1.0 + std::exp(-z));
}

double LogisticRegression::dot(const std::vector<double>& v1, const std::vector<double>& v2) const {
    double result = 0.0;
    for (size_t i = 0; i < v1.size(); i++) {
        result += v1[i] * v2[i];
    }
    return result;
}

void LogisticRegression::fit(const std::vector<std::vector<double>>& X,
    const std::vector<int>& y) {
    size_t n_samples = X.size();
    size_t n_features = X[0].size();
    coef_.assign(n_features, 0.0);
    intercept_ = 0.0;

    for (int iter = 0; iter < max_iter_; iter++) {
        std::vector<double> dw(n_features, 0.0);
        double db = 0.0;
        double loss = 0.0;

        for (size_t i = 0; i < n_samples; i++) {
            double linear_model = dot(X[i], coef_) + (fit_intercept_ ? intercept_ : 0.0);
            double y_pred = sigmoid(linear_model);

            double error = y_pred - y[i];
            for (size_t j = 0; j < n_features; j++) {
                dw[j] += X[i][j] * error;
            }
            if (fit_intercept_) {
                db += error;
            }

            // logistic loss
            loss += -y[i] * std::log(y_pred + 1e-12) - (1 - y[i]) * std::log(1 - y_pred + 1e-12);
        }

        // ��������
        if (penalty_ == "l2") {
            for (size_t j = 0; j < n_features; j++) {
                dw[j] += coef_[j] / C_;
            }
        }
        else if (penalty_ == "l1") {
            for (size_t j = 0; j < n_features; j++) {
                dw[j] += (coef_[j] > 0 ? 1 : -1) / C_;
            }
        }

        // ����
        for (size_t j = 0; j < n_features; j++) {
            coef_[j] -= learning_rate_ * dw[j] / n_samples;
        }
        if (fit_intercept_) {
            intercept_ -= learning_rate_ * db / n_samples;
        }

        if (loss / n_samples < tol_) {
            break;
        }
    }
}

void LogisticRegression::fitWithStandardization(const std::vector<std::vector<double>>& X,
                                               const std::vector<int>& y) {
    // 先标准化特征
    auto X_std = standardizeFeatures(X);
    // 然后训练模型
    fit(X_std, y);
}

std::vector<std::vector<double>> LogisticRegression::standardizeFeatures(
    const std::vector<std::vector<double>>& X) const {
    if (X.empty() || X[0].empty()) {
        return X;
    }
    
    size_t n_samples = X.size();
    size_t n_features = X[0].size();
    
    // 计算每个特征的均值和标准差
    std::vector<double> means(n_features, 0.0);
    std::vector<double> stds(n_features, 0.0);
    
    // 计算均值
    for (size_t j = 0; j < n_features; ++j) {
        for (size_t i = 0; i < n_samples; ++i) {
            means[j] += X[i][j];
        }
        means[j] /= n_samples;
    }
    
    // 计算标准差
    for (size_t j = 0; j < n_features; ++j) {
        for (size_t i = 0; i < n_samples; ++i) {
            double diff = X[i][j] - means[j];
            stds[j] += diff * diff;
        }
        stds[j] = std::sqrt(stds[j] / n_samples);
        // 避免除零
        if (stds[j] < 1e-10) {
            stds[j] = 1.0;
        }
    }
    
    // 标准化特征
    std::vector<std::vector<double>> X_std(n_samples, std::vector<double>(n_features));
    for (size_t i = 0; i < n_samples; ++i) {
        for (size_t j = 0; j < n_features; ++j) {
            X_std[i][j] = (X[i][j] - means[j]) / stds[j];
        }
    }
    
    return X_std;
}

std::vector<int> LogisticRegression::predict(const std::vector<std::vector<double>>& X) const {
    std::vector<int> preds;
    preds.reserve(X.size());
    for (const auto& sample : X) {
        double linear_model = dot(sample, coef_) + (fit_intercept_ ? intercept_ : 0.0);
        double prob = sigmoid(linear_model);
        preds.push_back(prob >= 0.5 ? 1 : 0);
    }
    return preds;
}

std::vector<std::vector<double>> LogisticRegression::predict_proba(const std::vector<std::vector<double>>& X) const {
    std::vector<std::vector<double>> probs;
    probs.reserve(X.size());
    for (const auto& sample : X) {
        double linear_model = dot(sample, coef_) + (fit_intercept_ ? intercept_ : 0.0);
        double p1 = sigmoid(linear_model);
        probs.push_back({ 1.0 - p1, p1 });
    }
    return probs;
}
