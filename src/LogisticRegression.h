#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#include <vector>
#include <string>

class LogisticRegression {
public:
    LogisticRegression(
        std::string penalty = "l2",
        double C = 1.0,
        bool fit_intercept = true,
        int max_iter = 100,
        double tol = 1e-4,
        double learning_rate = 0.01
    );

    void fit(const std::vector<std::vector<double>>& X,
        const std::vector<int>& y);

    std::vector<int> predict(const std::vector<std::vector<double>>& X) const;

    std::vector<std::vector<double>> predict_proba(const std::vector<std::vector<double>>& X) const;

    // 测试接口
    const std::vector<double>& get_coef() const { return coef_; }
    double get_intercept() const { return intercept_; }

    // 特征标准化
    void fitWithStandardization(const std::vector<std::vector<double>>& X,
                               const std::vector<int>& y);
    std::vector<std::vector<double>> standardizeFeatures(const std::vector<std::vector<double>>& X) const;

private:
    std::string penalty_;  // "none", "l2", "l1"
    double C_;
    bool fit_intercept_;
    int max_iter_;
    double tol_;
    double learning_rate_;

    std::vector<double> coef_;
    double intercept_;
    
    // 标准化参数
    std::vector<double> feature_means_;
    std::vector<double> feature_stds_;
    bool is_standardized_;

    // 内部函数
    double sigmoid(double z) const;
    double dot(const std::vector<double>& v1, const std::vector<double>& v2) const;
};

#endif // LOGISTIC_REGRESSION_H
