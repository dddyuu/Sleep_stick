#include "preprocessing.h"
#include <iostream>
#include<QDebug>
Preprocessing::Preprocessing()
{
	count = 0;
}

void Preprocessing::setSize(int row, int col)
{
	matrix.resize(row, col);
}

void Preprocessing::append(QList<double> data)
{
	for (int i=0; i < data.size(); i++)
	{
		matrix(count) = data[i];
		count++;
//        qDebug()<<"count:"<<count;
	}

	if (isReady())
	{

		Eigen::MatrixXd matrix=this->matrix(Eigen::seq(0, 169),0);
		//std::cout << "start processing\n";
		Eigen::MatrixXd input = decentration(matrix);
		input = covariance(input);
		input = decomposition(input);
		output=projection(input, matrix);
		count = 0;
	}

}

bool Preprocessing::isReady()
{
    if (count == matrix.rows())
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<float> Preprocessing::getOutput()
{
	std::vector<float> value = output;
	output.clear();
	return value;
}

Eigen::MatrixXd Preprocessing::decentration(Eigen::MatrixXd matrix)
{
	double mean = matrix.mean();
	for (int i = 0; i < matrix.rows(); i++)
	{
		matrix(i, 0) -= mean;
	}
	return matrix;
}

Eigen::MatrixXd Preprocessing::covariance(Eigen::MatrixXd value)
{
	Eigen::MatrixXd value_T = value.transpose();
	Eigen::MatrixXd output = value * value_T;
	return output;
}

Eigen::MatrixXd Preprocessing::decomposition(Eigen::MatrixXd value)
{
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(value);
	if (eigensolver.info() != Eigen::Success) {
		std::cerr << "Error during eigen decomposition." << std::endl;
		return Eigen::MatrixXd();
	}
	Eigen::VectorXd eigenvalues = eigensolver.eigenvalues();
	Eigen::MatrixXd eigenvectors = eigensolver.eigenvectors();
	
	std::vector<std::pair<double, int>> valueIndexPairs;
	for (int i = 0; i < eigenvalues.size(); ++i) {
		valueIndexPairs.emplace_back(eigenvalues[i], i);
	}
	 //按照向量的值进行降序排序
	std::sort(valueIndexPairs.begin(), valueIndexPairs.end(),
		[](const std::pair<double, int>& a, const std::pair<double, int>& b) {
			return a.first > b.first; // 降序排序
		});
	 //获取前 25 个最大值的索引
	std::vector<int> Indices;
	for (int i = 0; i < 25; ++i) {
		Indices.push_back(valueIndexPairs[i].second);
	}
	Eigen::MatrixXd P(eigenvectors.rows(), 25);
	for (int i = 0; i < 25; i++)
	{
		P.col(i) = eigenvectors.col(Indices[i]);
	}
	return P;
	return matrix;
}

std::vector<float> Preprocessing::projection(Eigen::MatrixXd value, Eigen::MatrixXd matrix)
{
	Eigen::MatrixXd output = value.transpose() * matrix;
	std::vector<float> result;
	for (int i = 0; i < output.rows(); i++)
	{
		result.push_back(output(i, 0));
	}
    return result;
}

// 计算平均值
float Preprocessing::calculateMean(const std::vector<float>& data) {
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

std::vector<float> Preprocessing::zScore(const std::vector<float>& data) {
    //assert(!data.empty()); // 确保数据非空

    float mean = calculateMean(data);
    float standardDeviation = calculateStandardDeviation(data, mean);

    std::vector<float> zScores;
    for (const auto& value : data) {
        double z = (value - mean) / standardDeviation;
        zScores.push_back(z);
    }
    return zScores;
}
// 计算标准差
float Preprocessing::calculateStandardDeviation(const std::vector<float>& data, float mean) {
    double varianceSum = 0.0;
    for (const auto& value : data) {
        varianceSum += (value - mean) * (value - mean);
    }
    double variance = varianceSum / data.size();
    return std::sqrt(variance);
}