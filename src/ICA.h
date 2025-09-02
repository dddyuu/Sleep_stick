#ifndef ICA_H
#define ICA_H

#include <Eigen/Dense>
#include <vector>
#include <utility>
#include <numeric> // <- 为 std::iota

class ICA {
public:
    explicit ICA(int n_components, int random_state = 97);

    // 拟合：X 维度 = [channels x samples]
    void fit(const Eigen::MatrixXd& X);

    // 变换到独立成分时序：返回 S = [n_components x samples]
    Eigen::MatrixXd transform(const Eigen::MatrixXd& X) const;

    // 设置需要排除的成分索引
    void set_exclude(const std::vector<int>& indices);

    // 应用 ICA 去除坏成分并重建到原始通道空间（返回 [channels x samples]）
    Eigen::MatrixXd apply(const Eigen::MatrixXd& X) const;

    // 根据 EOG 参考（可多通道，shape = [m x samples]）寻找坏成分
    // 返回：<坏成分下标, 各成分的最大绝对相关分数(与任一参考)>
    std::pair<std::vector<int>, std::vector<double>>
        find_bads_eog(const Eigen::MatrixXd& X,
            const Eigen::MatrixXd& eog_ref,
            double corr_threshold = 0.3) const;

private:
    int n_components_;
    int random_state_;

    // 模型参数
    Eigen::VectorXd mean_;          // [channels]
    Eigen::MatrixXd whitening_;     // [n_components x channels]
    Eigen::MatrixXd dewhitening_;   // [channels x n_components]
    Eigen::MatrixXd W_;             // [n_components x n_components]（白化空间的解混矩阵）
    Eigen::MatrixXd mixing_full_;   // [channels x n_components]    = dewhitening_ * W_.inverse()
    Eigen::MatrixXd unmixing_full_; // [n_components x channels]    = W_ * whitening_

    std::vector<int> exclude_;

    // 内部：对称 FastICA（输入已白化数据 Xw = [n_components x samples]）
    void fastica_symmetric(const Eigen::MatrixXd& X_white, Eigen::MatrixXd& W) const;

    // 内部：对称去相关 W -> (W * (W^T W)^(-1/2))
    static void symmetric_decorrelate(Eigen::MatrixXd& W);

    // 相关系数 |corr(x, y)|
    static double abs_pearson_corr(const Eigen::VectorXd& x, const Eigen::VectorXd& y);
};

#endif // ICA_H
