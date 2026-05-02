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
			return path;
	}
	
	// Fallback: try to find relative to home if it exists
	std::string home = Utils::Platform::getHomePath();
	if (Utils::FileSystem::isDirectory(home + "/roms"))
		return home + "/roms";

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

void ProfileManager::setActiveProfile(const std::string& name)
{
	if (Utils::FileSystem::exists(mProfilesRoot + "/" + name))
	{
		mActiveProfile = name;
		Settings::getInstance()->setString("ActiveProfile", name);
		Settings::getInstance()->saveFile();
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
