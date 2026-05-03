#include "RetroAchievements.h"
#include "HttpReq.h"
#include "ApiSystem.h"
#include "Settings.h"
#include "SystemConf.h"
#include "PlatformId.h"
#include "SystemData.h"
#include "FileData.h"
#include "ProfileManager.h"
#include "utils/Platform.h"
#include "utils/StringUtil.h"
#include "utils/ZipFile.h"
#include "Log.h"
#include <algorithm>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <libcheevos/cheevos.h>
#include "LocaleES.h"
#include "EmulationStation.h"

using namespace PlatformIds;

// (Console IDs and md5 set remain same as original)
const std::map<PlatformId, unsigned short> cheevosConsoleID = {
	{ ARCADE, RC_CONSOLE_ARCADE }, { NEOGEO, RC_CONSOLE_ARCADE }, { SEGA_MEGA_DRIVE, RC_CONSOLE_MEGA_DRIVE },
	{ NINTENDO_64, RC_CONSOLE_NINTENDO_64 }, { SUPER_NINTENDO, RC_CONSOLE_SUPER_NINTENDO }, { GAME_BOY, RC_CONSOLE_GAMEBOY },
	{ GAME_BOY_ADVANCE, RC_CONSOLE_GAMEBOY_ADVANCE }, { GAME_BOY_COLOR, RC_CONSOLE_GAMEBOY_COLOR }, { NINTENDO_ENTERTAINMENT_SYSTEM, RC_CONSOLE_NINTENDO },
	{ TURBOGRAFX_16, RC_CONSOLE_PC_ENGINE }, { SUPERGRAFX, RC_CONSOLE_PC_ENGINE }, { SEGA_CD, RC_CONSOLE_SEGA_CD },
	{ SEGA_32X, RC_CONSOLE_SEGA_32X }, { SEGA_MASTER_SYSTEM, RC_CONSOLE_MASTER_SYSTEM }, { PLAYSTATION, RC_CONSOLE_PLAYSTATION },
	{ ATARI_LYNX, RC_CONSOLE_ATARI_LYNX }, { NEOGEO_POCKET, RC_CONSOLE_NEOGEO_POCKET }, { SEGA_GAME_GEAR, RC_CONSOLE_GAME_GEAR },
	{ NINTENDO_GAMECUBE, RC_CONSOLE_GAMECUBE }, { ATARI_JAGUAR, RC_CONSOLE_ATARI_JAGUAR }, { NINTENDO_DS, RC_CONSOLE_NINTENDO_DS },
	{ NINTENDO_WII, RC_CONSOLE_WII }, { NINTENDO_WII_U, RC_CONSOLE_WII_U }, { PLAYSTATION_2, RC_CONSOLE_PLAYSTATION_2 },
	{ XBOX, RC_CONSOLE_XBOX }, { VIDEOPAC_ODYSSEY2, RC_CONSOLE_MAGNAVOX_ODYSSEY2 }, { POKEMINI, RC_CONSOLE_POKEMON_MINI },
	{ ATARI_2600, RC_CONSOLE_ATARI_2600 }, { PC, RC_CONSOLE_MS_DOS }, { NINTENDO_VIRTUAL_BOY, RC_CONSOLE_VIRTUAL_BOY },
	{ MSX, RC_CONSOLE_MSX }, { COMMODORE_64, RC_CONSOLE_COMMODORE_64 }, { ZX81, RC_CONSOLE_ZX81 }, { ORICATMOS, RC_CONSOLE_ORIC },
	{ SEGA_SG1000, RC_CONSOLE_SG1000 }, { AMIGA, RC_CONSOLE_AMIGA }, { ATARI_ST, RC_CONSOLE_ATARI_ST }, { AMSTRAD_CPC, RC_CONSOLE_AMSTRAD_PC },
	{ CREATONIC_MEGA_DUCK, RC_CONSOLE_MEGADUCK }, { APPLE_II, RC_CONSOLE_APPLE_II }, { SEGA_SATURN, RC_CONSOLE_SATURN },
	{ SEGA_DREAMCAST, RC_CONSOLE_DREAMCAST }, { PLAYSTATION_PORTABLE, RC_CONSOLE_PSP }, { THREEDO, RC_CONSOLE_3DO },
	{ COLECOVISION, RC_CONSOLE_COLECOVISION }, { INTELLIVISION, RC_CONSOLE_INTELLIVISION }, { VECTREX, RC_CONSOLE_VECTREX },
	{ PC_88, RC_CONSOLE_PC8800 }, { PC_98, RC_CONSOLE_PC9800 }, { PCFX, RC_CONSOLE_PCFX }, { ATARI_5200, RC_CONSOLE_ATARI_5200 },
	{ ATARI_7800, RC_CONSOLE_ATARI_7800 }, { SHARP_X6800, RC_CONSOLE_X68K }, { WONDERSWAN, RC_CONSOLE_WONDERSWAN },
	{ WASM4, RC_CONSOLE_WASM4 }, { NEOGEO_CD, RC_CONSOLE_NEO_GEO_CD }, { CHANNELF, RC_CONSOLE_FAIRCHILD_CHANNEL_F },
	{ ZX_SPECTRUM, RC_CONSOLE_ZX_SPECTRUM }, { NINTENDO_GAME_AND_WATCH, RC_CONSOLE_GAME_AND_WATCH }, { NINTENDO_3DS, RC_CONSOLE_NINTENDO_3DS },
	{ VIC20, RC_CONSOLE_VIC20 }, { SUPER_CASSETTE_VISION, RC_CONSOLE_SUPER_CASSETTEVISION }, { FMTOWNS, RC_CONSOLE_FM_TOWNS },
	{ NOKIA_NGAGE, RC_CONSOLE_NOKIA_NGAGE }, { PHILIPS_CDI, RC_CONSOLE_CDI }, { WATARA_SUPERVISION, RC_CONSOLE_SUPERVISION },
	{ SHARP_X1, RC_CONSOLE_SHARPX1 }, { TIC80, RC_CONSOLE_TIC80 }, { THOMSON_TO_MO, RC_CONSOLE_THOMSONTO8 },
	{ ARDUBOY, RC_CONSOLE_ARDUBOY }, { SUPER_NINTENDO_MSU1, RC_CONSOLE_SUPER_NINTENDO }, { EMERSON_ARCADIA_2001, RC_CONSOLE_ARCADIA_2001 },
	{ ATARI_JAGUAR_CD, RC_CONSOLE_ATARI_JAGUAR_CD }, { TURBOGRAFX_CD, RC_CONSOLE_PC_ENGINE_CD }, { UZEBOX, RC_CONSOLE_UZEBOX }
};

const std::set<unsigned short> consolesWithmd5hashes = {
	RC_CONSOLE_APPLE_II, RC_CONSOLE_ATARI_2600, RC_CONSOLE_ATARI_JAGUAR, RC_CONSOLE_COLECOVISION, RC_CONSOLE_GAMEBOY,
	RC_CONSOLE_GAMEBOY_ADVANCE, RC_CONSOLE_GAMEBOY_COLOR, RC_CONSOLE_GAME_GEAR, RC_CONSOLE_INTELLIVISION,
	RC_CONSOLE_MAGNAVOX_ODYSSEY2, RC_CONSOLE_MASTER_SYSTEM, RC_CONSOLE_MEGA_DRIVE, RC_CONSOLE_MSX,
	RC_CONSOLE_NEOGEO_POCKET, RC_CONSOLE_ORIC, RC_CONSOLE_PC8800, RC_CONSOLE_POKEMON_MINI, RC_CONSOLE_SEGA_32X,
	RC_CONSOLE_SG1000, RC_CONSOLE_VECTREX, RC_CONSOLE_VIRTUAL_BOY, RC_CONSOLE_WONDERSWAN, RC_CONSOLE_SUPERVISION
};

static HttpReqOptions getHttpOptions() {
	HttpReqOptions options;
#ifdef CHEEVOS_DEV_LOGIN
	std::string ret = Utils::String::extractString(CHEEVOS_DEV_LOGIN, "z=", "&");
	ret =  ret + "/" + Utils::String::replace(RESOURCE_VERSION_STRING, ",", ".");		 
	options.userAgent = ret;
#endif	
	return options;
}

bool RetroAchievements::sLocalEngineActive = false;

bool RetroAchievements::isLocalEngineActive() {
	if (sLocalEngineActive) return true;
	std::string hubPath = getRetroAchievementsHubPath();
	if (hubPath.empty()) return false;

	// Use getShOutput to check for running process
	std::string output = Utils::Platform::getShOutput("pidof LAHEE");
	sLocalEngineActive = !output.empty();
	if (sLocalEngineActive) {
		LOG(LogInfo) << "LAHEE heartbeat confirmed (PID: " << output << ")";
	}
	return sLocalEngineActive;
}

int jsonInt(const rapidjson::Value& val, const std::string& name) {
	if (!val.HasMember(name.c_str())) return 0;
	const rapidjson::Value& v = val[name.c_str()];
	if (v.IsInt()) return v.GetInt();
	if (v.IsString()) return Utils::String::toInteger(v.GetString());
	return 0;
}

std::string jsonString(const rapidjson::Value& val, const std::string& name) {
	if (!val.HasMember(name.c_str())) return "";
	const rapidjson::Value& v = val[name.c_str()];
	if (v.IsInt()) return std::to_string(v.GetInt());
	if (v.IsString()) return v.GetString();
	return "";
}

std::string RetroAchievements::getRetroAchievementsHubPath() {
	for (auto pSystem : SystemData::sSystemVector) {
		if (!pSystem->isCollection()) {
			std::string romsRoot = Utils::FileSystem::getParent(pSystem->getStartPath());
			std::string hubPath = romsRoot + "/RetroAchievements";
			if (Utils::FileSystem::exists(hubPath)) return hubPath;
		}
	}
	return "";
}

int RetroAchievements::getLocalGameId(FileData* file) {
	std::string hubPath = getRetroAchievementsHubPath();
	if (hubPath.empty()) return 0;
	std::string md5 = file->getMetadata(MetaDataId::Md5);
	if (md5.empty()) {
		md5 = getCheevosHash(file->getSourceFileData()->getSystem(), file->getPath());
		if (md5 == "00000000000000000000000000000000") return 0;
	}
	std::string dataDir = hubPath + "/Data";
	auto files = Utils::FileSystem::getDirContent(dataDir);
	for (auto fileName : files) {
		if (Utils::FileSystem::getExtension(fileName) == ".txt" && fileName.find(".hash") != std::string::npos) {
			std::string content = Utils::FileSystem::readAllText(fileName);
			if (content.find(md5) != std::string::npos) {
				std::string stem = Utils::FileSystem::getStem(fileName);
				size_t pos = stem.find(".");
				if (pos != std::string::npos) return Utils::String::toInteger(stem.substr(0, pos));
			}
		}
	}
	return 0;
}

UserSummary RetroAchievements::getUserSummary(const std::string& userName, int gameCount) {
	auto usrName = userName;
	if (usrName.empty()) usrName = SystemConf::getInstance()->get("global.retroachievements.username");
	if (usrName.empty()) usrName = "Player";
	UserSummary ret;
	ret.Username = usrName;
	ret.Rank = "LOCAL";
	ret.Points = "0";

	// NATIVE FILE SYSTEM ENGINE
	if (isLocalEngineActive()) {
		std::string userFile = getRetroAchievementsHubPath() + "/User/" + usrName + ".json";
		if (Utils::FileSystem::exists(userFile)) {
			rapidjson::Document doc;
			doc.Parse(Utils::FileSystem::readAllText(userFile).c_str());
			if (!doc.HasParseError() && doc.IsObject()) {
				ret.Points = jsonString(doc, "Points");
				
				if (doc.HasMember("RecentlyPlayed") && doc["RecentlyPlayed"].IsArray()) {
					for (auto& gp : doc["RecentlyPlayed"].GetArray()) {
						RecentGame item;
						item.GameID = jsonString(gp, "GameID");
						item.Title = jsonString(gp, "Title");
						item.ImageIcon = jsonString(gp, "ImageIcon");
						ret.RecentlyPlayed.push_back(item);
					}
				}
				ret.RecentlyPlayedCount = (int)ret.RecentlyPlayed.size();

				if (doc.HasMember("Awarded") && doc["Awarded"].IsObject()) {
					for (auto it = doc["Awarded"].MemberBegin(); it != doc["Awarded"].MemberEnd(); ++it) {
						Award item;
						item.NumPossibleAchievements = jsonInt(it->value, "NumPossibleAchievements");
						item.PossibleScore = jsonInt(it->value, "PossibleScore");
						item.NumAchieved = jsonInt(it->value, "NumAchieved");
						item.ScoreAchieved = jsonInt(it->value, "ScoreAchieved");
						item.NumAchievedHardcore = jsonInt(it->value, "NumAchievedHardcore");
						item.ScoreAchievedHardcore = jsonInt(it->value, "ScoreAchievedHardcore");
						ret.Awarded[it->name.GetString()] = item;
					}
				}
				return ret;
			}
		}
	}

	// Legacy Network Engine
	auto options = getHttpOptions();
	HttpReq httpreq(getApiUrl("API_GetUserSummary", "u="+ HttpReq::urlEncode(usrName) +"&g=10&a=10"), &options);
	if (httpreq.wait()) {
		rapidjson::Document doc;
		doc.Parse(httpreq.getContent().c_str());
		if (!doc.HasParseError()) {
			ret.Points = jsonString(doc, "Points");
			ret.Rank = jsonString(doc, "Rank");
			ret.UserPic = jsonString(doc, "UserPic");
		}
	}
	return ret;
}

GameInfoAndUserProgress RetroAchievements::getGameInfoAndUserProgress(int gameId, const std::string& userName) {
	auto usrName = userName;
	if (usrName.empty()) usrName = SystemConf::getInstance()->get("global.retroachievements.username");
	GameInfoAndUserProgress ret;
	ret.ID = gameId;

	if (isLocalEngineActive() && gameId > 0) {
		std::string setFile = getRetroAchievementsHubPath() + "/Data/" + std::to_string(gameId) + ".set.json";
		if (Utils::FileSystem::exists(setFile)) {
			rapidjson::Document doc;
			doc.Parse(Utils::FileSystem::readAllText(setFile).c_str());
			if (!doc.HasParseError() && doc.IsObject()) {
				ret.Title = jsonString(doc, "Title");
				ret.ImageIcon = jsonString(doc, "ImageIcon");
				if (doc.HasMember("Achievements") && doc["Achievements"].IsArray()) {
					for (auto& achiv : doc["Achievements"].GetArray()) {
						Achievement item;
						item.ID = jsonString(achiv, "ID");
						item.Title = jsonString(achiv, "Title");
						item.Description = jsonString(achiv, "Description");
						item.Points = jsonString(achiv, "Points");
						item.BadgeName = jsonString(achiv, "BadgeName");
						ret.Achievements.push_back(item);
					}
				}
				return ret;
			}
		}
	}
	return ret;
}

std::string RetroAchievements::getApiUrl(const std::string& method, const std::string& parameters) {
	std::string serverUrl = Settings::getInstance()->getString("RetroAchievementsServerURL");
	if (serverUrl.empty()) serverUrl = "https://retroachievements.org/API/";
	if (serverUrl.find("/laheer/") != std::string::npos) {
		std::string m = Utils::String::toLower(method);
		if (Utils::String::startsWith(m, "api_")) m = m.substr(4);
		if (m == "getuserrankandscore") m = "getusersummary";
		return serverUrl + "dorequest.php?r=" + m + "&" + parameters;
	}
	return serverUrl + method + ".php?" + parameters;
}

std::string GameInfoAndUserProgress::getImageUrl(const std::string& image) {
	std::string serverUrl = Settings::getInstance()->getString("RetroAchievementsServerURL");
	if (serverUrl.find("/laheer/") != std::string::npos) {
		std::string img = image.empty() ? ImageIcon : image;
		if (img.empty()) return "";
		if (img[0] == '/') img = img.substr(1);
		return serverUrl + img;
	}
	return "http://i.retroachievements.org" + (image.empty() ? ImageIcon : image);
}

std::string Achievement::getBadgeUrl() {
	std::string serverUrl = Settings::getInstance()->getString("RetroAchievementsServerURL");
	if (serverUrl.find("/laheer/") != std::string::npos) {
		return serverUrl + "Images/" + BadgeName + (DateEarned.empty() && DateEarnedHardcore.empty() ? "_lock.png" : ".png");
	}
	return "http://i.retroachievements.org/Badge/" + BadgeName + (DateEarned.empty() && DateEarnedHardcore.empty() ? "_lock.png" : ".png");
}

RetroAchievementInfo RetroAchievements::toRetroAchivementInfo(UserSummary& ret) {
	RetroAchievementInfo info;
	info.username = ret.Username;
	info.points = ret.Points;
	info.rank = ret.Rank;
	return info;
}

std::map<std::string, std::string> RetroAchievements::getCheevosHashes() {
	return std::map<std::string, std::string>(); // Native mode uses getLocalGameId
}

std::string RetroAchievements::getCheevosHashFromFile(int consoleId, const std::string& fileName) {
	char hash[33];
	if (generateHashFromFile(hash, consoleId, fileName.c_str())) return hash;
	return "00000000000000000000000000000000";	
}

std::string RetroAchievements::getCheevosHash(SystemData* system, const std::string& fileName) {
	int consoleId = 0;
	if (system->getPlatformIds().size() > 0) {
		auto it = cheevosConsoleID.find(*system->getPlatformIds().begin());
		if (it != cheevosConsoleID.end()) consoleId = it->second;
	}
	
	return getCheevosHashFromFile(consoleId, fileName);
}

bool RetroAchievements::testAccount(const std::string& username, const std::string& password, std::string& tokenOrError) {
	if (Settings::getInstance()->getString("RetroAchievementsServerURL").find("127.0.0.1") != std::string::npos) {
		tokenOrError = "local";
		return true;
	}
	return false;
}

void RetroAchievements::updateRetroArchConfig() {
	std::string selected = ProfileManager::getInstance()->getActiveProfile();
	if (selected.empty()) selected = "Player";

	std::vector<std::string> cfgDirs = {
		"/home/ark/.config/retroarch/",
		"/home/ark/.config/retroarch32/",
		"/storage/.config/retroarch/",
		"/userdata/system/configs/retroarch/"
	};

	std::string saveDir = ProfileManager::getInstance()->getSavePath();
	std::string stateDir = ProfileManager::getInstance()->getStatePath();
	std::string screenshotDir = ProfileManager::getInstance()->getScreenshotPath();

	for (const auto& dir : cfgDirs) {
		if (!Utils::FileSystem::isDirectory(dir)) continue;

		auto files = Utils::FileSystem::getDirContent(dir);
		for (const auto& path : files) {
			if (!Utils::String::endsWith(path, ".cfg")) continue;

			LOG(LogInfo) << "Injecting Profile [" << selected << "] into " << path;

			// ROBUST INJECTION: Use a temporary script to avoid shell escaping hell.
			std::string script = "sed -i '/cheevos_username/d' \"" + path + "\"\n" +
			                     "echo 'cheevos_username = \"" + selected + "\"' >> \"" + path + "\"\n" +
			                     "sed -i '/cheevos_password/d' \"" + path + "\"\n" +
			                     "echo 'cheevos_password = \"lahee\"' >> \"" + path + "\"\n" +
			                     "sed -i '/cheevos_token/d' \"" + path + "\"\n" +
			                     "echo 'cheevos_token = \"\"' >> \"" + path + "\"\n" +
			                     "sed -i '/cheevos_custom_host/d' \"" + path + "\"\n" +
			                     "echo 'cheevos_custom_host = \"http://127.0.0.1:8000/laheer/\"' >> \"" + path + "\"\n" +
			                     "sed -i '/cheevos_enable/d' \"" + path + "\"\n" +
			                     "echo 'cheevos_enable = \"true\"' >> \"" + path + "\"\n" +
			                     "sed -i '/savefile_directory/d' \"" + path + "\"\n" +
			                     "echo 'savefile_directory = \"" + saveDir + "\"' >> \"" + path + "\"\n" +
			                     "sed -i '/savestate_directory/d' \"" + path + "\"\n" +
			                     "echo 'savestate_directory = \"" + stateDir + "\"' >> \"" + path + "\"\n" +
			                     "sed -i '/screenshot_directory/d' \"" + path + "\"\n" +
			                     "echo 'screenshot_directory = \"" + screenshotDir + "\"' >> \"" + path + "\"\n" +
			                     "sed -i '/savestates_in_content_dir/d' \"" + path + "\"\n" +
			                     "echo 'savestates_in_content_dir = \"false\"' >> \"" + path + "\"\n" +
			                     "sed -i '/savefiles_in_content_dir/d' \"" + path + "\"\n" +
			                     "echo 'savefiles_in_content_dir = \"false\"' >> \"" + path + "\"\n";

			Utils::FileSystem::writeAllText("/tmp/inject_ra.sh", script);
			Utils::Platform::getShOutput("sh /tmp/inject_ra.sh");
		}
	}
}
