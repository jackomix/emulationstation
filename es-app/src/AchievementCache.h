#pragma once

#include <string>
#include <vector>
#include <map>
#include "RetroAchievements.h"

class AchievementCache
{
public:
	static void init();

	// Read/Write global game definitions
	static bool hasGameData(int gameId);
	static bool loadGameData(int gameId, GameInfoAndUserProgress& outGameInfo);
	static void saveGameData(int gameId, const std::string& jsonData);

	// Read/Write per-profile user progress
	static bool loadUserProgress(int gameId, GameInfoAndUserProgress& inOutGameInfo);
	static void saveUserProgress(int gameId, const GameInfoAndUserProgress& gameInfo);

	// User summary cache
	static bool loadUserSummary(std::string& outJson);
	static void saveUserSummary(const std::string& jsonData);

	static const std::map<std::string, std::string>& loadHashMap();

private:
	static std::string getGlobalAchievementsPath();
	static std::string getUserProgressPath();
};
