// preFcm.cpp 修改
#include "preFcm.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <random>
#include <Eigen/Dense>

preFcm::preFcm(int clusters, double m, QObject* parent)
    : QObject(parent), clusters(clusters), m(m), U(Eigen::MatrixXd::Zero(clusters, 0)) {
    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
}

void preFcm::setCenters(const std::vector<std::vector<double>>& centers) {
    if (centers.size() != static_cast<size_t>(clusters)) {
        throw std::invalid_argument("聚类中心数量不匹配");
    }
    this->centers = centers;
}

std::vector<std::vector<double>> preFcm::getCenters() const {
    return centers;
}

Eigen::MatrixXd preFcm::getMembershipMatrix() const {
    return U;
}

void preFcm::train(const std::vector<std::vector<float>>& data, int max_iters, double epsilon) {
    if (data.empty()) return;

    const int n = data.size();       // 样本数
    const int dim = data[0].size();  // 特征维度

    trainData = data;
    U = Eigen::MatrixXd::Zero(clusters, n);

    // 1. 随机初始化隶属度矩阵
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    for (int i = 0; i < clusters; ++i) {
        for (int k = 0; k < n; ++k) {
            U(i, k) = dis(gen);
        }
    }

    // 归一化使每列和为1
    for (int k = 0; k < n; ++k) {
        double sum = 0.0;
        for (int i = 0; i < clusters; ++i) {
            sum += U(i, k);
        }
        for (int i = 0; i < clusters; ++i) {
            U(i, k) /= sum;
        }
    }

    // 迭代优化
    Eigen::MatrixXd prevU;
    for (int iter = 0; iter < max_iters; ++iter) {
        prevU = U;

        // 2. 更新聚类中心
        centers.clear();
        centers.resize(clusters, std::vector<double>(dim, 0.0));

        for (int i = 0; i < clusters; ++i) {
            double denominator = 0.0;
            std::vector<double> numerator(dim, 0.0);

            for (int k = 0; k < n; ++k) {
                double um = std::pow(U(i, k), m);
                denominator += um;

                for (int d = 0; d < dim; ++d) {
                    numerator[d] += um * data[k][d];
                }
            }

            for (int d = 0; d < dim; ++d) {
                centers[i][d] = numerator[d] / denominator;
            }
        }

        // 3. 更新隶属度
        for (int i = 0; i < clusters; ++i) {
            for (int k = 0; k < n; ++k) {
                double dist_ik = 0.0;
                for (int d = 0; d < dim; ++d) {
                    double diff = data[k][d] - centers[i][d];
                    dist_ik += diff * diff;
                }
                dist_ik = std::sqrt(dist_ik);

                double sum = 0.0;
                for (int j = 0; j < clusters; ++j) {
                    double dist_jk = 0.0;
                    for (int d = 0; d < dim; ++d) {
                        double diff = data[k][d] - centers[j][d];
                        dist_jk += diff * diff;
                    }
                    dist_jk = std::sqrt(dist_jk);

                    if (dist_jk < 1e-10) {
                        sum = (i == j) ? 1.0 : 0.0;
                        break;
                    }
                    sum += std::pow(dist_ik / dist_jk, 2.0 / (m - 1.0));
                }

                U(i, k) = 1.0 / sum;
            }
        }

        // 4. 检查收敛条件
        double max_diff = (U - prevU).cwiseAbs().maxCoeff();
        if (max_diff < epsilon) {
            break;
        }
    }
}

std::vector<float> preFcm::computeMembership(const std::vector<float>& sample) {
    if (centers.empty()) {
        throw std::runtime_error("聚类中心未初始化");
    }

    std::vector<float> membership(clusters, 0.0f);
    std::vector<double> distances(clusters, 0.0);

    // 计算到各聚类中心的距离
    for (int i = 0; i < clusters; ++i) {
        double dist = 0.0;
        for (size_t j = 0; j < sample.size(); ++j) {
            double diff = static_cast<double>(sample[j]) - centers[i][j];
            dist += diff * diff;
        }
        distances[i] = (dist < 1e-10) ? 1e-10 : std::sqrt(dist);
    }

    // 计算隶属度
    for (int i = 0; i < clusters; ++i) {
        double sum = 0.0;
        for (int k = 0; k < clusters; ++k) {
            sum += std::pow(distances[i] / distances[k], 2.0 / (m - 1.0));
        }
        membership[i] = static_cast<float>(1.0 / sum);
    }

    return membership;
}

std::vector<std::vector<double>> preFcm::computeFuzzifiedFeatures() const {
    if (centers.empty() || U.size() == 0) {
        throw std::runtime_error("模型未训练");
    }

    // 将聚类中心转为Eigen矩阵 (clusters x dim)
    Eigen::MatrixXd C(clusters, centers[0].size());
    for (int i = 0; i < clusters; ++i) {
        for (size_t j = 0; j < centers[i].size(); ++j) {
            C(i, j) = centers[i][j];
        }
    }

    // 计算模糊化特征: X = (C * U)^T
    Eigen::MatrixXd X = (C * U).transpose();

    // 转为标准vector
    std::vector<std::vector<double>> fuzzifiedFeatures;
    fuzzifiedFeatures.reserve(X.rows());

    for (int i = 0; i < X.rows(); ++i) {
        std::vector<double> row;
        row.reserve(X.cols());
        for (int j = 0; j < X.cols(); ++j) {
            row.push_back(X(i, j));
        }
        fuzzifiedFeatures.push_back(row);
    }

    return fuzzifiedFeatures;
}