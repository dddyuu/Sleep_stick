#ifndef PREPROCESSING_H
#define PREPROCESSING_H
#include "Dense"

#include "QObject"
class Preprocessing:public QObject
{
public:
	Preprocessing();
	void setSize(int row,int col);
	void append(QList<double>);
	bool isReady();
    std::vector<float> getOutput();//有无输出
	std::vector<float> zScore(const std::vector<float>& data);
private:
	Eigen::MatrixXd matrix;
	int count;
	Eigen::MatrixXd decentration(Eigen::MatrixXd);
	Eigen::MatrixXd covariance(Eigen::MatrixXd);
	Eigen::MatrixXd decomposition(Eigen::MatrixXd);
	std::vector<float>  projection(Eigen::MatrixXd, Eigen::MatrixXd);
	std::vector<float> output;
    //模型输入需要这里归一化
    float calculateMean(const std::vector<float>& data);
    
    float calculateStandardDeviation(const std::vector<float>& data, float mean);
};
#endif
