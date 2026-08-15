#pragma once
#include <string>

namespace ddlc
{
	struct Result
	{
		bool ok = false;
		std::string text;
	};

	Result CompleteEverything();
	Result LockEverything();
	Result SaveNow();
}