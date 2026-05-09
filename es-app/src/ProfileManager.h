#pragma once
#ifndef ES_APP_PROFILE_MANAGER_H
#define ES_APP_PROFILE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

struct ProfileStats {
	int playtime;
	std::string last_played;
	std::string most_played_genre;
};

struct ProfileInfo {
	std::string id;
	std::string name;
};

class Window;

class ProfileManager
{
public:
	static ProfileManager* getInstance();

	void init();
	
	std::string getProfilesPath();
	std::string getActiveProfile();
	std::string getActiveProfileId();
	void setActiveProfile(const std::string& id);

	bool createProfile(const std::string& name);
	bool deleteProfile(const std::string& id);
	bool renameProfile(const std::string& id, const std::string& newName);

	std::vector<ProfileInfo> getAvailableProfiles();

	// Sandboxed Paths
	std::string getSavePath(const std::string& profile = "");
	std::string getStatePath(const std::string& profile = "");
	std::string getScreenshotPath(const std::string& profile = "");
	std::string getFavoritesPath(const std::string& profile = "");
	std::string getAvatarPath(const std::string& profile = "");

	bool isFavorite(const std::string& path);
	void setFavorite(const std::string& path, bool favorite);
	
	std::string getStat(const std::string& key);
	void updateStats(const std::string& romPath, int playTimeSeconds);

	void loadAllMetadata();
	void saveAllMetadata();

	void switchProfileAsync(Window* window, const std::string& name, std::function<void()> onComplete);

private:
	ProfileManager();
	static ProfileManager* sInstance;

	std::string mActiveProfile;
	std::string mProfilesRoot;
	
	std::unordered_set<std::string> mFavoritesCache;
	ProfileStats mStatsCache;

	void setupDefaultProfile();
	void migrateLegacySaves();
	std::string findRomsRoot();
};

#endif // ES_APP_PROFILE_MANAGER_H
