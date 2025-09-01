#include "tskfatigue.h"
#include <iostream>
#include <random>
int main()
{
	Calculate::TskFatigue tskfatigue;
	std::vector<float> input;
	std::random_device rd;  // 用于种子生成的随机设备
	std::mt19937 gen(rd()); // 使用随机设备初始化 Mersenne Twister 引擎

	// 创建一个均匀分布的浮点数生成器，范围在 0 到 1 之间
	std::uniform_real_distribution<> dis(0.0, 1.0);

	// 生成一个 0 到 1 之间的随机数
	
	for (int i = 0; i < 25; i++)
	{
		double random_value = dis(gen);
		input.push_back(random_value);
	}
	tskfatigue.run(input);
	std::vector<std::vector<float>>  output =tskfatigue.getOutputValue();
	for (int i = 0; i < output.size(); i++)
	{
		for (int j = 0; j < 2; j++)
		{
			std::cout << output[i][j] << " ";
		}
		std::cout << std::endl;
	}
}