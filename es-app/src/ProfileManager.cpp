#include "ProfileManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/Platform.h"
#include "Settings.h"
#include "Log.h"
#include <algorithm>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

ProfileManager* ProfileManager::sInstance = nullptr;

ProfileManager* ProfileManager::getInstance()
{
	if (sInstance == nullptr)
		sInstance = new ProfileManager();
	return sInstance;
}

ProfileManager::ProfileManager()
{
	mProfilesRoot = "";
	mActiveProfile = "";
}

std::string ProfileManager::findRomsRoot()
{
	std::vector<std::string> searchPaths = { "/roms", "/storage/roms", "/userdata/roms", "/media/sdcard/roms" };
	for (const auto& path : searchPaths)
	{
		if (Utils::FileSystem::isDirectory(path))
			return Utils::FileSystem::getCanonicalPath(path);
	}
	
	std::string home = Utils::FileSystem::getGenericPath("~");
	if (Utils::FileSystem::isDirectory(home + "/roms"))
		return Utils::FileSystem::getCanonicalPath(home + "/roms");

	return "/roms"; 
}

void ProfileManager::init()
{
	std::string root = findRomsRoot();
	mProfilesRoot = root + "/Profiles";

	if (!Utils::FileSystem::exists(mProfilesRoot))
	{
		Utils::FileSystem::createDirectory(mProfilesRoot);
		setupDefaultProfile();
		migrateLegacySaves();
	}

	mActiveProfile = Settings::getInstance()->getString("ActiveProfile");
	if (mActiveProfile.empty() || !Utils::FileSystem::exists(mProfilesRoot + "/" + mActiveProfile))
	{
		auto available = getAvailableProfiles();
		mActiveProfile = (!available.empty() ? available[0] : "Player");
	}

	loadAllMetadata();
}

void ProfileManager::loadAllMetadata()
{
	// 1. Favorites
	mFavoritesCache.clear();
	std::string favPath = getFavoritesPath();
	if (Utils::FileSystem::exists(favPath))
	{
		std::string content = Utils::FileSystem::readAllText(favPath);
		std::stringstream ss(content);
		std::string line;
		while (std::getline(ss, line))
		{
			if (!line.empty()) mFavoritesCache.insert(line);
		}
	}

	// 2. Stats
	mStatsCache = {0, "", ""};
	std::string statsPath = mProfilesRoot + "/" + mActiveProfile + "/stats.json";
	if (Utils::FileSystem::exists(statsPath))
	{
		rapidjson::Document doc;
		doc.Parse(Utils::FileSystem::readAllText(statsPath).c_str());
		if (!doc.HasParseError() && doc.IsObject())
		{
			if (doc.HasMember("playtime") && doc["playtime"].IsInt()) mStatsCache.playtime = doc["playtime"].GetInt();
			if (doc.HasMember("last_played") && doc["last_played"].IsString()) mStatsCache.last_played = doc["last_played"].GetString();
			if (doc.HasMember("most_played_genre") && doc["most_played_genre"].IsString()) mStatsCache.most_played_genre = doc["most_played_genre"].GetString();
		}
	}
}

void ProfileManager::saveAllMetadata()
{
	// 1. Favorites (Atomic Save)
	std::string favContent = "";
	for (const auto& fav : mFavoritesCache) favContent += fav + "\n";
	
	std::string favPath = getFavoritesPath();
	std::string favTemp = favPath + ".tmp";
	Utils::FileSystem::writeAllText(favTemp, favContent);
	Utils::FileSystem::renameFile(favTemp, favPath);

	// 2. Stats (Atomic Save)
	rapidjson::Document doc;
	doc.SetObject();
	
	auto& allocator = doc.GetAllocator();
	doc.AddMember("playtime", mStatsCache.playtime, allocator);
	doc.AddMember("last_played", rapidjson::Value(mStatsCache.last_played.c_str(), allocator).Move(), allocator);
	doc.AddMember("most_played_genre", rapidjson::Value(mStatsCache.most_played_genre.c_str(), allocator).Move(), allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	std::string statsPath = mProfilesRoot + "/" + mActiveProfile + "/stats.json";
	std::string statsTemp = statsPath + ".tmp";
	Utils::FileSystem::writeAllText(statsTemp, buffer.GetString());
	Utils::FileSystem::renameFile(statsTemp, statsPath);
}

bool ProfileManager::isFavorite(const std::string& path)
{
	return mFavoritesCache.find(path) != mFavoritesCache.end();
}

void ProfileManager::setFavorite(const std::string& path, bool favorite)
{
	if (favorite) mFavoritesCache.insert(path);
	else mFavoritesCache.erase(path);
	saveAllMetadata();
}

std::string ProfileManager::getStat(const std::string& key)
{
	if (key == "playtime") return std::to_string(mStatsCache.playtime);
	if (key == "last_played") return mStatsCache.last_played;
	if (key == "most_played_genre") return mStatsCache.most_played_genre;
	return "";
}

void ProfileManager::updateStats(const std::string& romPath, int playTimeSeconds)
{
	mStatsCache.playtime += playTimeSeconds;
	mStatsCache.last_played = Utils::FileSystem::getFileName(romPath);
	saveAllMetadata();
}

void ProfileManager::setActiveProfile(const std::string& name)
{
	if (Utils::FileSystem::exists(mProfilesRoot + "/" + name))
	{
		mActiveProfile = name;
		Settings::getInstance()->setString("ActiveProfile", name);
		Settings::getInstance()->saveFile();

		Utils::FileSystem::createDirectory(getSavePath(name));
		Utils::FileSystem::createDirectory(getStatePath(name));
		Utils::FileSystem::createDirectory(getScreenshotPath(name));

		loadAllMetadata();
	}
}

void ProfileManager::setupDefaultProfile()
{
	createProfile("Player");
	mActiveProfile = "Player";
	Settings::getInstance()->setString("ActiveProfile", "Player");
	Settings::getInstance()->saveFile();
}

bool ProfileManager::createProfile(const std::string& name)
{
	if (name.empty()) return false;
	std::string path = mProfilesRoot + "/" + name;
	if (Utils::FileSystem::exists(path)) return false;

	Utils::FileSystem::createDirectory(path);
	Utils::FileSystem::createDirectory(path + "/Saves");
	Utils::FileSystem::createDirectory(path + "/States");
	Utils::FileSystem::createDirectory(path + "/Screenshots");
	
	Utils::FileSystem::writeAllText(path + "/favorites.txt", "");
	Utils::FileSystem::writeAllText(path + "/stats.json", "{\"playtime\": 0, \"last_played\": \"\", \"most_played_genre\": \"\"}");
	
	return true;
}

std::vector<std::string> ProfileManager::getAvailableProfiles()
{
	std::vector<std::string> profiles;
	auto content = Utils::FileSystem::getDirContent(mProfilesRoot);
	for (const auto& path : content)
	{
		if (Utils::FileSystem::isDirectory(path))
			profiles.push_back(Utils::FileSystem::getFileName(path));
	}
	return profiles;
}

bool ProfileManager::deleteProfile(const std::string& name)
{
	if (name == "Player" && getAvailableProfiles().size() == 1) return false;
	std::string path = mProfilesRoot + "/" + name;
	if (Utils::FileSystem::exists(path))
	{
		std::string cmd = "rm -rf \"" + path + "\"";
		Utils::Platform::getShOutput("sh -c '" + cmd + "'");
		if (mActiveProfile == name) {
			auto available = getAvailableProfiles();
			setActiveProfile(!available.empty() ? available[0] : "Player");
		}
		return true;
	}
	return false;
}

bool ProfileManager::renameProfile(const std::string& oldName, const std::string& newName)
{
	std::string oldPath = mProfilesRoot + "/" + oldName;
	std::string newPath = mProfilesRoot + "/" + newName;
	if (Utils::FileSystem::exists(oldPath) && !Utils::FileSystem::exists(newPath))
	{
		std::string cmd = "mv \"" + oldPath + "\" \"" + newPath + "\"";
		Utils::Platform::getShOutput("sh -c '" + cmd + "'");
		if (mActiveProfile == oldName) mActiveProfile = newName;
		return true;
	}
	return false;
}

void ProfileManager::migrateLegacySaves()
{
	std::string root = findRomsRoot();
	std::vector<std::pair<std::string, std::string>> migrations = {
		{ root + "/saves", getSavePath("Player") },
		{ root + "/savestates", getStatePath("Player") }
	};
	for (auto const& [source, target] : migrations)
	{
		if (Utils::FileSystem::isDirectory(source))
		{
			std::string cmd = "mv \"" + source + "/*\" \"" + target + "/\" 2>/dev/null";
			Utils::Platform::getShOutput("sh -c '" + cmd + "'");
		}
	}
}

std::string ProfileManager::getProfilesPath() { return mProfilesRoot; }
std::string ProfileManager::getSavePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Saves"; }
std::string ProfileManager::getStatePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/States"; }
std::string ProfileManager::getScreenshotPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Screenshots"; }
std::string ProfileManager::getFavoritesPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/favorites.txt"; }
std::string ProfileManager::getAvatarPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/avatar.png"; }
