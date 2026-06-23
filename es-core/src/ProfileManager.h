#pragma once

#include <string>
#include <vector>

struct Profile {
	std::string name;
	std::string avatarPath;
};

class ProfileManager
{
public:
	static ProfileManager* getInstance();

	bool isProfilesEnabled();
	void setProfilesEnabled(bool enabled);
	
	std::vector<Profile> getProfiles();
	Profile getActiveProfile();
	std::string getActiveProfileName();
	std::string getProfileDataPath();
	std::string getProfileDataPath(const std::string& name);

	void loadProfiles();
	void saveProfiles();
	bool createProfile(const std::string& name, const std::string& avatarPath = "");
	bool deleteProfile(const std::string& name);
	bool renameProfile(const std::string& oldName, const std::string& newName);
	void setActiveProfile(const std::string& name);
	
	std::string getProfilesRoot();

private:
	ProfileManager();
	static ProfileManager* sInstance;

	std::vector<Profile> mProfiles;
	std::string mActiveProfileName;
	bool mProfilesEnabled;

	void createProfileDirectoryStructure(const std::string& name);
};
