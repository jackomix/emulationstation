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

#include <thread>
#include "HttpReq.h"
#include "views/ViewController.h"
#include "Window.h"

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
	mActiveProfile = "1"; // Default to ID 1
}

std::string ProfileManager::findRomsRoot()
{
	std::vector<std::string> searchPaths = { "/roms", "/storage/roms", "/userdata/roms", "/media/sdcard/roms" };
	for (const auto& path : searchPaths)
		if (Utils::FileSystem::isDirectory(path))
			return Utils::FileSystem::getCanonicalPath(path);
	
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
		createProfile("Player"); // ID 1
	}

	mActiveProfile = Settings::getInstance()->getString("ActiveProfileId");
	if (mActiveProfile.empty() || !Utils::FileSystem::exists(mProfilesRoot + "/" + mActiveProfile))
	{
		auto available = getAvailableProfiles();
		mActiveProfile = (!available.empty() ? available[0].id : "1");
	}

	loadAllMetadata();
}

void ProfileManager::loadAllMetadata()
{
	mFavoritesCache.clear();
	std::string favPath = getFavoritesPath();
	if (Utils::FileSystem::exists(favPath))
	{
		std::string content = Utils::FileSystem::readAllText(favPath);
		std::stringstream ss(content);
		std::string line;
		while (std::getline(ss, line))
			if (!line.empty()) mFavoritesCache.insert(line);
	}

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
	std::string favContent = "";
	for (const auto& fav : mFavoritesCache) favContent += fav + "\n";
	std::string favPath = getFavoritesPath();
	Utils::FileSystem::writeAllText(favPath + ".tmp", favContent);
	Utils::FileSystem::renameFile(favPath + ".tmp", favPath);

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
	Utils::FileSystem::writeAllText(statsPath + ".tmp", buffer.GetString());
	Utils::FileSystem::renameFile(statsPath + ".tmp", statsPath);
}

void ProfileManager::switchProfileAsync(Window* window, const std::string& id, std::function<void()> onComplete)
{
	setActiveProfile(id);

	std::thread([window, id, onComplete]() {
		// Get display name for the ID
		std::string name = "Player";
		std::string path = ProfileManager::getInstance()->getProfilesPath() + "/" + id + "/profile.json";
		if (Utils::FileSystem::exists(path)) {
			rapidjson::Document doc;
			doc.Parse(Utils::FileSystem::readAllText(path).c_str());
			if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("display_name"))
				name = doc["display_name"].GetString();
		}

		HttpReqOptions options;
		// Send BOTH ID and Name to LAHEE
		std::string url = "http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&id=" + id + "&u=" + HttpReq::urlEncode(name);
		HttpReq request(url, &options);
		
		for (int i = 0; i < 20; i++) {
			if (request.status() != HttpReq::REQ_IN_PROGRESS) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		
		if (window && onComplete) window->postToUiThread([onComplete]() { onComplete(); });
	}).detach();
}

bool ProfileManager::isFavorite(const std::string& path) { return mFavoritesCache.find(path) != mFavoritesCache.end(); }

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

void ProfileManager::setActiveProfile(const std::string& id)
{
	if (Utils::FileSystem::exists(mProfilesRoot + "/" + id))
	{
		mActiveProfile = id;
		Settings::getInstance()->setString("ActiveProfileId", id);
		Settings::getInstance()->saveFile();
		loadAllMetadata();
	}
}

std::string ProfileManager::getActiveProfile() 
{ 
	std::string path = mProfilesRoot + "/" + mActiveProfile + "/profile.json";
	if (Utils::FileSystem::exists(path)) {
		rapidjson::Document doc;
		doc.Parse(Utils::FileSystem::readAllText(path).c_str());
		if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("display_name"))
			return doc["display_name"].GetString();
	}
	return "Player";
}

std::string ProfileManager::getActiveProfileId() { return mActiveProfile; }

bool ProfileManager::createProfile(const std::string& name)
{
	if (name.empty()) return false;
	
	// 1. Find next numeric ID
	int maxId = 0;
	auto content = Utils::FileSystem::getDirContent(mProfilesRoot);
	for (const auto& path : content) {
		if (Utils::FileSystem::isDirectory(path)) {
			int id = atoi(Utils::FileSystem::getFileName(path).c_str());
			if (id > maxId) maxId = id;
		}
	}
	std::string newId = std::to_string(maxId + 1);
	std::string path = mProfilesRoot + "/" + newId;

	Utils::FileSystem::createDirectory(path);
	Utils::FileSystem::createDirectory(path + "/Saves");
	Utils::FileSystem::createDirectory(path + "/States");
	Utils::FileSystem::createDirectory(path + "/Screenshots");
	Utils::FileSystem::createDirectory(path + "/Achievements");
	
	// 2. Write Identity
	Utils::FileSystem::writeAllText(path + "/profile.json", "{\"display_name\": \"" + name + "\"}");
	Utils::FileSystem::writeAllText(path + "/favorites.txt", "");
	Utils::FileSystem::writeAllText(path + "/stats.json", "{\"playtime\": 0, \"last_played\": \"\", \"most_played_genre\": \"\"}");
	
	return true;
}

std::vector<ProfileInfo> ProfileManager::getAvailableProfiles()
{
	std::vector<ProfileInfo> profiles;
	auto content = Utils::FileSystem::getDirContent(mProfilesRoot);
	for (const auto& path : content)
	{
		if (Utils::FileSystem::isDirectory(path)) {
			ProfileInfo info;
			info.id = Utils::FileSystem::getFileName(path);
			
			std::string pJson = path + "/profile.json";
			if (Utils::FileSystem::exists(pJson)) {
				rapidjson::Document doc;
				doc.Parse(Utils::FileSystem::readAllText(pJson).c_str());
				if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("display_name"))
					info.name = doc["display_name"].GetString();
				else info.name = "Profile " + info.id;
			} else {
				info.name = "Profile " + info.id;
			}
			profiles.push_back(info);
		}
	}
	return profiles;
}

bool ProfileManager::deleteProfile(const std::string& id)
{
	if (id == "1" && getAvailableProfiles().size() == 1) return false;
	std::string path = mProfilesRoot + "/" + id;
	if (Utils::FileSystem::exists(path))
	{
		std::string cmd = "rm -rf \"" + path + "\"";
		Utils::Platform::getShOutput("sh -c '" + cmd + "'");
		return true;
	}
	return false;
}

bool ProfileManager::renameProfile(const std::string& id, const std::string& newName)
{
	std::string path = mProfilesRoot + "/" + id + "/profile.json";
	if (Utils::FileSystem::exists(path)) {
		Utils::FileSystem::writeAllText(path, "{\"display_name\": \"" + newName + "\"}");
		return true;
	}
	return false;
}

void ProfileManager::migrateLegacySaves() { }

std::string ProfileManager::getProfilesPath() { return mProfilesRoot; }
std::string ProfileManager::getSavePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Saves"; }
std::string ProfileManager::getStatePath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/States"; }
std::string ProfileManager::getScreenshotPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/Screenshots"; }
std::string ProfileManager::getFavoritesPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/favorites.txt"; }
std::string ProfileManager::getAvatarPath(const std::string& profile) { return mProfilesRoot + "/" + (profile.empty() ? mActiveProfile : profile) + "/avatar.png"; }
