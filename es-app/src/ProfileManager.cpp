#include "ProfileManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/Platform.h"
#include "Settings.h"
#include "Log.h"
#include <algorithm>

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
	// Common mount points for ArkOS, AmberELEC, JELOS, etc.
	std::vector<std::string> searchPaths = { "/roms", "/storage/roms", "/userdata/roms", "/media/sdcard/roms" };
	for (const auto& path : searchPaths)
	{
		if (Utils::FileSystem::isDirectory(path))
			return Utils::FileSystem::getCanonicalPath(path);
	}
	
	// Fallback: try to find relative to home if it exists
	std::string home = Utils::FileSystem::getGenericPath("~");
	if (Utils::FileSystem::isDirectory(home + "/roms"))
		return Utils::FileSystem::getCanonicalPath(home + "/roms");

	return "/roms"; // Default fallback
}

void ProfileManager::init()
{
	std::string root = findRomsRoot();
	mProfilesRoot = root + "/Profiles";

	if (!Utils::FileSystem::exists(mProfilesRoot))
	{
		LOG(LogInfo) << "Profiles folder missing. Creating root at: " << mProfilesRoot;
		Utils::FileSystem::createDirectory(mProfilesRoot);
		setupDefaultProfile();
		migrateLegacySaves();
	}

	mActiveProfile = Settings::getInstance()->getString("ActiveProfile");
	if (mActiveProfile.empty() || !Utils::FileSystem::exists(mProfilesRoot + "/" + mActiveProfile))
	{
		auto available = getAvailableProfiles();
		if (!available.empty())
			mActiveProfile = available[0];
		else
			setupDefaultProfile();
	}

	loadFavorites();
}

void ProfileManager::loadFavorites()
{
	mFavorites.clear();
	std::string path = getFavoritesPath();
	if (!Utils::FileSystem::exists(path)) return;

	std::string content = Utils::FileSystem::readAllText(path);
	std::stringstream ss(content);
	std::string line;
	while (std::getline(ss, line))
	{
		if (!line.empty()) mFavorites.push_back(line);
	}
}

void ProfileManager::saveFavorites()
{
	std::string path = getFavoritesPath();
	std::string content = "";
	for (const auto& fav : mFavorites) content += fav + "\n";
	Utils::FileSystem::writeAllText(path, content);
}

bool ProfileManager::isFavorite(const std::string& path)
{
	return std::find(mFavorites.begin(), mFavorites.end(), path) != mFavorites.end();
}

void ProfileManager::setFavorite(const std::string& path, bool favorite)
{
	auto it = std::find(mFavorites.begin(), mFavorites.end(), path);
	if (favorite && it == mFavorites.end())
	{
		mFavorites.push_back(path);
		saveFavorites();
	}
	else if (!favorite && it != mFavorites.end())
	{
		mFavorites.erase(it);
		saveFavorites();
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
	
	// Init Metadata
	Utils::FileSystem::writeAllText(path + "/favorites.txt", "");
	Utils::FileSystem::writeAllText(path + "/stats.json", "{\"playtime\": 0, \"last_played\": \"\", \"most_played_genre\": \"\"}");
	
	LOG(LogInfo) << "Created new profile: " << name;
	return true;
}

void ProfileManager::migrateLegacySaves()
{
	std::string root = findRomsRoot();
	std::string targetSaves = mProfilesRoot + "/Player/Saves";
	std::string targetStates = mProfilesRoot + "/Player/States";

	// Migration targets (Common legacy paths)
	std::vector<std::pair<std::string, std::string>> migrations = {
		{ root + "/saves", targetSaves },
		{ root + "/savestates", targetStates }
	};

	for (auto const& [source, target] : migrations)
	{
		if (Utils::FileSystem::isDirectory(source))
		{
			LOG(LogInfo) << "Migrating legacy data from " << source << " to " << target;
			// Use shell for recursive move to handle nested console folders
			std::string cmd = "mv \"" + source + "/*\" \"" + target + "/\" 2>/dev/null";
			Utils::Platform::getShOutput("sh -c '" + cmd + "'");
		}
	}
}

std::string ProfileManager::getActiveProfile() { return mActiveProfile; }

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

void ProfileManager::setActiveProfile(const std::string& name)
{
	if (Utils::FileSystem::exists(mProfilesRoot + "/" + name))
	{
		mActiveProfile = name;
		Settings::getInstance()->setString("ActiveProfile", name);
		Settings::getInstance()->saveFile();

		// Ensure subfolders exist (fixes RetroArch redirect fail)
		Utils::FileSystem::createDirectory(getSavePath(name));
		Utils::FileSystem::createDirectory(getStatePath(name));
		Utils::FileSystem::createDirectory(getScreenshotPath(name));
	}
}

std::vector<std::string> ProfileManager::getAvailableProfiles()
{
	std::vector<std::string> profiles;
	auto content = Utils::FileSystem::getDirContent(mProfilesRoot);
	for (const auto& path : content)
	{
		if (Utils::FileSystem::isDirectory(path))
		{
			profiles.push_back(Utils::FileSystem::getFileName(path));
		}
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
		
		if (mActiveProfile == name)
		{
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

std::string ProfileManager::getProfilesPath() { return mProfilesRoot; }
std::string ProfileManager::getSavePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Saves"; }
std::string ProfileManager::getStatePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/States"; }
std::string ProfileManager::getScreenshotPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Screenshots"; }
std::string ProfileManager::getFavoritesPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/favorites.txt"; }
std::string ProfileManager::getAvatarPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/avatar.png"; }

std::string ProfileManager::getStat(const std::string& key)
{
	std::string path = mProfilesRoot + "/" + mActiveProfile + "/stats.json";
	if (!Utils::FileSystem::exists(path)) return "";

	rapidjson::Document doc;
	doc.Parse(Utils::FileSystem::readAllText(path).c_str());
	if (doc.HasParseError() || !doc.IsObject()) return "";

	if (doc.HasMember(key.c_str()))
	{
		if (doc[key.c_str()].IsString()) return doc[key.c_str()].GetString();
		if (doc[key.c_str()].IsInt()) return std::to_string(doc[key.c_str()].GetInt());
	}
	return "";
}

void ProfileManager::updateStats(const std::string& romPath, int playTimeSeconds)
{
	std::string path = mProfilesRoot + "/" + mActiveProfile + "/stats.json";
	rapidjson::Document doc;
	
	std::string content = Utils::FileSystem::exists(path) ? Utils::FileSystem::readAllText(path) : "{}";
	doc.Parse(content.c_str());
	if (doc.HasParseError() || !doc.IsObject()) doc.SetObject();

	// Update playtime
	int currentPlayTime = 0;
	if (doc.HasMember("playtime")) currentPlayTime = doc["playtime"].GetInt();
	
	if (!doc.HasMember("playtime")) doc.AddMember("playtime", currentPlayTime + playTimeSeconds, doc.GetAllocator());
	else doc["playtime"].SetInt(currentPlayTime + playTimeSeconds);

	// Update last played
	std::string lastPlayed = Utils::FileSystem::getFileName(romPath);
	if (!doc.HasMember("last_played")) doc.AddMember("last_played", rapidjson::Value(lastPlayed.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
	else doc["last_played"].SetString(lastPlayed.c_str(), doc.GetAllocator());

	// Save back
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	Utils::FileSystem::writeAllText(path, buffer.GetString());
}
