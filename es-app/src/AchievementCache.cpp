#include "AchievementCache.h"
#include "ProfileManager.h"
#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

std::string AchievementCache::getGlobalAchievementsPath()
{
	return "/roms/achievements";
}

std::string AchievementCache::getUserProgressPath()
{
	if (ProfileManager::getInstance()->isProfilesEnabled()) {
		return ProfileManager::getInstance()->getProfileDataPath() + "/achievements/progress";
	}
	return getGlobalAchievementsPath() + "/progress";
}

void AchievementCache::init()
{
	Utils::FileSystem::createDirectory(getGlobalAchievementsPath());
	Utils::FileSystem::createDirectory(getGlobalAchievementsPath() + "/games");
	Utils::FileSystem::createDirectory(getGlobalAchievementsPath() + "/badges");
	Utils::FileSystem::createDirectory(getUserProgressPath());
}

bool AchievementCache::hasGameData(int gameId)
{
	std::string path = getGlobalAchievementsPath() + "/games/" + std::to_string(gameId) + ".json";
	return Utils::FileSystem::exists(path);
}

bool AchievementCache::loadGameData(int gameId, GameInfoAndUserProgress& outGameInfo)
{
	std::string path = getGlobalAchievementsPath() + "/games/" + std::to_string(gameId) + ".json";
	if (!Utils::FileSystem::exists(path))
		return false;

	std::string json = Utils::FileSystem::readAllText(path);
	rapidjson::Document doc;
	doc.Parse(json.c_str());
	
	if (doc.HasParseError() || !doc.IsObject())
		return false;

	// Populate basic game info (the RetroAchievements::getGameInfoAndUserProgress logic expects doc to have these fields)
	// We will parse the full JSON in RetroAchievements.cpp where the logic already exists.
	// For this cache layer, we just verify the file exists and let RetroAchievements parse it.
	return true;
}

void AchievementCache::saveGameData(int gameId, const std::string& jsonData)
{
	init();
	std::string path = getGlobalAchievementsPath() + "/games/" + std::to_string(gameId) + ".json";
	Utils::FileSystem::writeAllText(path, jsonData);
}

bool AchievementCache::loadUserProgress(int gameId, GameInfoAndUserProgress& inOutGameInfo)
{
	std::string path = getUserProgressPath() + "/" + std::to_string(gameId) + ".json";
	if (!Utils::FileSystem::exists(path))
		return false;

	std::string json = Utils::FileSystem::readAllText(path);
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (doc.HasParseError() || !doc.IsObject())
		return false;

	if (doc.HasMember("achievements") && doc["achievements"].IsObject()) {
		const auto& achObj = doc["achievements"];
		
		int awardedCount = 0;
		int hardcoreCount = 0;
		
		for (auto& ach : inOutGameInfo.Achievements) {
			if (achObj.HasMember(ach.ID.c_str())) {
				const auto& prog = achObj[ach.ID.c_str()];
				if (prog.HasMember("unlocked") && prog["unlocked"].GetBool()) {
					if (prog.HasMember("dateEarned")) ach.DateEarned = prog["dateEarned"].GetString();
					if (prog.HasMember("hardcore") && prog["hardcore"].GetBool()) {
						ach.DateEarnedHardcore = ach.DateEarned;
						hardcoreCount++;
					}
					awardedCount++;
				}
			}
		}
		inOutGameInfo.NumAwardedToUser = awardedCount;
		inOutGameInfo.NumAwardedToUserHardcore = hardcoreCount;
		
		if (inOutGameInfo.NumAchievements > 0) {
			inOutGameInfo.UserCompletion = std::to_string((awardedCount * 100) / inOutGameInfo.NumAchievements) + "%";
			inOutGameInfo.UserCompletionHardcore = std::to_string((hardcoreCount * 100) / inOutGameInfo.NumAchievements) + "%";
		}
	}

	return true;
}

void AchievementCache::saveUserProgress(int gameId, const GameInfoAndUserProgress& gameInfo)
{
	init();
	std::string path = getUserProgressPath() + "/" + std::to_string(gameId) + ".json";
	
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

	doc.AddMember("gameId", gameId, allocator);
	
	rapidjson::Value achObj(rapidjson::kObjectType);
	for (const auto& ach : gameInfo.Achievements) {
		rapidjson::Value progObj(rapidjson::kObjectType);
		bool unlocked = !ach.DateEarned.empty();
		bool hardcore = !ach.DateEarnedHardcore.empty();
		
		progObj.AddMember("unlocked", unlocked, allocator);
		if (unlocked) {
			progObj.AddMember("dateEarned", rapidjson::Value(ach.DateEarned.c_str(), allocator), allocator);
			progObj.AddMember("hardcore", hardcore, allocator);
		}
		
		rapidjson::Value key(ach.ID.c_str(), allocator);
		achObj.AddMember(key, progObj, allocator);
	}
	doc.AddMember("achievements", achObj, allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	Utils::FileSystem::writeAllText(path, buffer.GetString());
}

bool AchievementCache::loadUserSummary(UserSummary& outSummary)
{
	std::string path = getUserProgressPath() + "/user.json";
	if (!Utils::FileSystem::exists(path))
		return false;

	// In a full implementation, we would deserialize this from our cached JSON.
	// For Phase 1, we just return false if it's missing.
	return false;
}

void AchievementCache::saveUserSummary(const std::string& jsonData)
{
	init();
	std::string path = getUserProgressPath() + "/user.json";
	Utils::FileSystem::writeAllText(path, jsonData);
}
