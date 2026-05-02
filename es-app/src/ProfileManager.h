#pragma once
#ifndef ES_APP_PROFILE_MANAGER_H
#define ES_APP_PROFILE_MANAGER_H

#include <string>
#include <vector>
#include <memory>

class ProfileManager
{
public:
	static ProfileManager* getInstance();

	void init();
	
	std::string getProfilesPath();
	std::string getActiveProfile();
	void setActiveProfile(const std::string& name);

	bool createProfile(const std::string& name);
	bool deleteProfile(const std::string& name);
	bool renameProfile(const std::string& oldName, const std::string& newName);

	std::vector<std::string> getAvailableProfiles();

	// Sandboxed Paths
	std::string getSavePath(const std::string& profile = "");
	std::string getStatePath(const std::string& profile = "");
	std::string getScreenshotPath(const std::string& profile = "");
	std::string getFavoritesPath(const std::string& profile = "");
	std::string getAvatarPath(const std::string& profile = "");

private:
	ProfileManager();
	static ProfileManager* sInstance;

	std::string mActiveProfile;
	std::string mProfilesRoot;

	void setupDefaultProfile();
	void migrateLegacySaves();
	std::string findRomsRoot();
};

#endif // ES_APP_PROFILE_MANAGER_H
