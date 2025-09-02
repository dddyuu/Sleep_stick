#ifndef TSKFATIGUE_H
#define TSKFATIGUE_H
#include "base.h"
namespace Calculate {
	class TskFatigue;
}
class Calculate::TskFatigue:public Calculate::DeepLearn::Base
{
public:
	TskFatigue();
	~TskFatigue();
private:
	Ort::Session* model;
	std::vector<int64_t> input_size;
	std::vector<const char*> input_names;
	std::vector<int64_t> output_size;
	std::vector<const char*> output_names;
	std::vector<Ort::Value> output_value;
	
};
#endif // !TSKFATIGUE_H
