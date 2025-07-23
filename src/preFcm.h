// preFcm.h 修改
#pragma once

#include <QObject>
#include <vector>
#include <cmath>
#include <random>
#include <stdexcept>
#include <Eigen/Dense>  // 添加Eigen库支持矩阵运算

class preFcm : public QObject {
    Q_OBJECT
public:
    explicit preFcm(int clusters = 25, double m = 2.0, QObject* parent = nullptr); // 默认聚类数改为25
    void setCenters(const std::vector<std::vector<double>>& centers);
    std::vector<float> computeMembership(const std::vector<float>& sample);
    std::vector<std::vector<double>> getCenters() const;
    Eigen::MatrixXd getMembershipMatrix() const;

    // 添加训练函数
    void train(const std::vector<std::vector<float>>& data, int max_iters = 100, double epsilon = 1e-5);

    // 添加矩阵运算
    std::vector<std::vector<double>> computeFuzzifiedFeatures() const;

private:
    int clusters;  // 聚类数
    double m;      // 模糊指数
    std::vector<std::vector<double>> centers;  // 聚类中心
    Eigen::MatrixXd U;  // 隶属度矩阵
    std::vector<std::vector<float>> trainData; // 存储训练数据
};