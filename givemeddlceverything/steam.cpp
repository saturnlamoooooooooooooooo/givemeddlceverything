#include "steam.h"
#include <Windows.h>

namespace steam
{
	namespace
	{
		using ISteamUserStats = void;

		ISteamUserStats* (*UserStats)() = nullptr;
		bool (*SetAchievement)(ISteamUserStats*, const char*) = nullptr;
		bool (*StoreStats)(ISteamUserStats*) = nullptr;
	}

	bool Load(std::string& error)
	{
		HMODULE api = GetModuleHandleA("steam_api64.dll");
		if (api == nullptr)
		{
			error = "steam_api64.dll isn't loaded - launch through Steam.";
			return false;
		}

		UserStats = reinterpret_cast<decltype(UserStats)>(GetProcAddress(api, "SteamAPI_SteamUserStats_v012"));
		SetAchievement = reinterpret_cast<decltype(SetAchievement)>(GetProcAddress(api, "SteamAPI_ISteamUserStats_SetAchievement"));
		StoreStats = reinterpret_cast<decltype(StoreStats)>(GetProcAddress(api, "SteamAPI_ISteamUserStats_StoreStats"));

		if (UserStats == nullptr || SetAchievement == nullptr || StoreStats == nullptr)
		{
			error = "steam_api64.dll is missing the flat API exports we need.";
			return false;
		}

		return true;
	}

	int GrantAll(const std::vector<std::string>& achievementIds, std::string& error)
	{
		if (UserStats == nullptr)
		{
			error = "Steam not loaded.";
			return 0;
		}

		ISteamUserStats* stats = UserStats();
		if (stats == nullptr)
		{
			error = "Steam returned no ISteamUserStats - is Steam running?";
			return 0;
		}

		int granted = 0;
		for (const std::string& id : achievementIds)
		{
			if (SetAchievement(stats, id.c_str()))
			{
				granted++;
			}
		}

		StoreStats(stats);
		return granted;
	}
}