#include "ProfileManager.h"
#include "utils/FileSystemUtil.h"
#include "Paths.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>
#include "utils/StringUtil.h"
#include <algorithm>
#include "Settings.h"

ProfileManager* ProfileManager::sInstance = nullptr;

ProfileManager* ProfileManager::getInstance()
{
	if (sInstance == nullptr)
		sInstance = new ProfileManager();

	return sInstance;
}

ProfileManager::ProfileManager() : mActiveProfileName("")
{
	loadProfiles();
}

std::string ProfileManager::getProfilesRoot()
{
	if (Utils::FileSystem::isDirectory("/roms")) {
		return "/roms/profiles";
	}
	return Paths::getUserEmulationStationPath() + "/profiles";
}

bool ProfileManager::isProfilesEnabled()
{
	return true;
}

void ProfileManager::setProfilesEnabled(bool enabled)
{
	// Deprecated, profiles are always enabled
}

std::vector<Profile> ProfileManager::getProfiles()
{
	return mProfiles;
}

Profile ProfileManager::getActiveProfile()
{
	for (const auto& p : mProfiles) {
		if (p.name == mActiveProfileName) {
			return p;
		}
	}
	Profile empty = {"", ""};
	return empty;
}

std::string ProfileManager::getActiveProfileName()
{
	return mActiveProfileName;
}

std::string ProfileManager::getProfileDataPath()
{
	if (mActiveProfileName.empty()) return "";
	return getProfileDataPath(mActiveProfileName);
}

std::string ProfileManager::getProfileDataPath(const std::string& name)
{
	return getProfilesRoot() + "/" + name;
}

void ProfileManager::loadProfiles()
{
	mProfiles.clear();
	std::string root = getProfilesRoot();
	std::string path = root + "/profiles.xml";
	if (!Utils::FileSystem::exists(path)) {
		createProfile("Player 1");
		mActiveProfileName = "Player 1";
		return;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(WINSTRINGW(path).c_str());
	if (!result) {
		LOG(LogError) << "Could not parse profiles.xml file!\n   " << result.description();
		createProfile("Player 1");
		mActiveProfileName = "Player 1";
		return;
	}

	pugi::xml_node rootNode = doc.child("profiles");
	if (!rootNode) {
		createProfile("Player 1");
		mActiveProfileName = "Player 1";
		return;
	}

	mActiveProfileName = rootNode.attribute("active").as_string("");

	for (pugi::xml_node node = rootNode.child("profile"); node; node = node.next_sibling("profile")) {
		Profile p;
		p.name = node.attribute("name").as_string("");
		p.avatarPath = node.attribute("avatar").as_string("");
		if (!p.name.empty()) {
			mProfiles.push_back(p);
		}
	}

	// Validate active profile
	if (!mActiveProfileName.empty()) {
		bool found = false;
		for (const auto& p : mProfiles) {
			if (p.name == mActiveProfileName) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (!mProfiles.empty()) {
				mActiveProfileName = mProfiles[0].name;
			} else {
				createProfile("Player 1");
				mActiveProfileName = "Player 1";
			}
		}
	} else if (!mProfiles.empty()) {
		mActiveProfileName = mProfiles[0].name;
	} else {
		createProfile("Player 1");
		mActiveProfileName = "Player 1";
	}
}

void ProfileManager::saveProfiles()
{
	std::string root = getProfilesRoot();
	if (!Utils::FileSystem::exists(root)) {
		Utils::FileSystem::createDirectory(root);
	}
	std::string path = root + "/profiles.xml";

	pugi::xml_document doc;
	pugi::xml_node rootNode = doc.append_child("profiles");
	rootNode.append_attribute("enabled").set_value(true);
	rootNode.append_attribute("active").set_value(mActiveProfileName.c_str());

	for (const auto& p : mProfiles) {
		pugi::xml_node node = rootNode.append_child("profile");
		node.append_attribute("name").set_value(p.name.c_str());
		node.append_attribute("avatar").set_value(p.avatarPath.c_str());
	}

	doc.save_file(WINSTRINGW(path).c_str());
}

bool ProfileManager::createProfile(const std::string& name, const std::string& avatarPath)
{
	// Sanitize name: alphanumeric, dash, underscore only
	std::string cleanName = "";
	for (char c : name) {
		if (isalnum(c) || c == '-' || c == '_') {
			cleanName += c;
		}
	}
	if (cleanName.empty()) return false;

	// Check for duplicates
	for (const auto& p : mProfiles) {
		if (p.name == cleanName) return false;
	}

	Profile p;
	p.name = cleanName;
	p.avatarPath = avatarPath;
	mProfiles.push_back(p);

	createProfileDirectoryStructure(cleanName);

	// Auto-enable if it's the first profile
	if (mProfiles.size() == 1) {
		mActiveProfileName = cleanName;
		mProfilesEnabled = true;
		Paths::recalculateProfilePaths();
		Settings::getInstance()->loadFile();
	}

	saveProfiles();
	return true;
}

void ProfileManager::createProfileDirectoryStructure(const std::string& name)
{
	std::string base = getProfileDataPath(name);
	Utils::FileSystem::createDirectory(base);
	Utils::FileSystem::createDirectory(base + "/saves");
	Utils::FileSystem::createDirectory(base + "/savestates");
	Utils::FileSystem::createDirectory(base + "/screenshots");
	Utils::FileSystem::createDirectory(base + "/gamelists");
}

bool ProfileManager::deleteProfile(const std::string& name)
{
	for (auto it = mProfiles.begin(); it != mProfiles.end(); ++it) {
		if (it->name == name) {
			mProfiles.erase(it);

			if (mActiveProfileName == name) {
				if (!mProfiles.empty()) {
					mActiveProfileName = mProfiles[0].name;
				} else {
					mActiveProfileName = "";
					mProfilesEnabled = false;
				}
				Paths::recalculateProfilePaths();
				Settings::getInstance()->loadFile();
			}

			std::string path = getProfileDataPath(name);
			Utils::FileSystem::deleteDirectoryFiles(path, true);

			saveProfiles();
			return true;
		}
	}
	return false;
}

bool ProfileManager::renameProfile(const std::string& oldName, const std::string& newName)
{
	std::string cleanName = "";
	for (char c : newName) {
		if (isalnum(c) || c == '-' || c == '_') {
			cleanName += c;
		}
	}
	if (cleanName.empty()) return false;

	for (const auto& p : mProfiles) {
		if (p.name == cleanName) return false;
	}

	bool found = false;
	for (auto& p : mProfiles) {
		if (p.name == oldName) {
			p.name = cleanName;
			found = true;
			break;
		}
	}

	if (!found) return false;

	std::string oldPath = getProfileDataPath(oldName);
	std::string newPath = getProfileDataPath(cleanName);

	if (Utils::FileSystem::exists(oldPath)) {
		Utils::FileSystem::renameFile(oldPath, newPath);
	}

	if (mActiveProfileName == oldName) {
		mActiveProfileName = cleanName;
		Paths::recalculateProfilePaths();
		Settings::getInstance()->loadFile();
	}

	saveProfiles();
	return true;
}

void ProfileManager::setActiveProfile(const std::string& name)
{
	for (const auto& p : mProfiles) {
		if (p.name == name) {
			mActiveProfileName = name;
			saveProfiles();
			Paths::recalculateProfilePaths();
			Settings::getInstance()->loadFile();
			return;
		}
	}
}
