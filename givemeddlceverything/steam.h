#pragma once
#include <string>
#include <vector>

namespace steam
{
	bool Load(std::string& error);
	int GrantAll(const std::vector<std::string>& achievementIds, std::string& error);
}