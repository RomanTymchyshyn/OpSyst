#pragma once
#include <string>
#include <vector>
#include <map>

class CLParser
{
public:

	CLParser(int argc_, char * argv_[], bool switches_on_ = false);
	~CLParser() {}

	std::string get_arg(int i);
	std::string get_arg(std::string s);

private:

	int argc;
	std::vector<std::string> argv;

	bool switches_on;
	std::map<std::string, std::string> switch_map;
};
