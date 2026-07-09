#include "AchievementServer.h"
#include "services/httplib.h"
#include "Paths.h"
#include "utils/FileSystemUtil.h"
#include "AchievementCache.h"
#include "RetroAchievements.h"
#include <thread>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <ctime>
#include "Log.h"

static std::unique_ptr<httplib::Server> sServer;
static std::thread sThread;
static bool sRunning = false;

void AchievementServer::start()
{
	if (sRunning) return;
	
	sServer = std::make_unique<httplib::Server>();
	
	auto handler = [](const httplib::Request& req, httplib::Response& res) {
		LOG(LogDebug) << "AchievementServer dorequest.php invoked with params:";
		for (auto& param : req.params) {
			LOG(LogDebug) << "  " << param.first << " = " << param.second;
		}

		if (req.has_param("r")) {
			std::string action = req.get_param_value("r");
			LOG(LogInfo) << "AchievementServer action: " << action;
		}
		std::map<std::string, std::string> postParams;
		if (!req.body.empty()) {
			std::string body = req.body;
			size_t pos = 0;
			while ((pos = body.find('&')) != std::string::npos) {
				std::string pair = body.substr(0, pos);
				size_t eqPos = pair.find('=');
				if (eqPos != std::string::npos) {
					postParams[pair.substr(0, eqPos)] = pair.substr(eqPos + 1);
				}
				body.erase(0, pos + 1);
			}
			size_t eqPos = body.find('=');
			if (eqPos != std::string::npos) {
				postParams[body.substr(0, eqPos)] = body.substr(eqPos + 1);
			}
		}

		auto getParam = [&](const std::string& key) -> std::string {
			if (req.has_param(key.c_str())) return req.get_param_value(key.c_str());
			if (postParams.find(key) != postParams.end()) return postParams[key];
			return "";
		};

		std::string r = getParam("r");
		if (r.empty()) {
			res.status = 400;
			return;
		}

		
		if (r == "gameid") {
			std::string hash = getParam("m");
			std::string hashesPath = Paths::getGlobalAchievementsPath() + "/hashes.json";
			int gameId = 0;
			if (Utils::FileSystem::exists(hashesPath)) {
				std::string json = Utils::FileSystem::readAllText(hashesPath);
				rapidjson::Document doc;
				doc.Parse(json.c_str());
				if (!doc.HasParseError() && doc.HasMember(hash.c_str())) {
					gameId = doc[hash.c_str()].GetInt();
				}
			}
			res.set_content("{\"Success\":true,\"GameID\":" + std::to_string(gameId) + "}", "application/json");
		}
		else if (r == "ping") {
			res.set_content("{\"Success\":true}", "application/json");
		}
		else if (r == "submitlbentry") {
			res.set_content("{\"Success\":true,\"Response\":{\"Score\":0,\"BestScore\":0,\"RankInfo\":{\"Rank\":1,\"NumEntries\":1}}}", "application/json");
		}
		else if (r == "achievementsets") {
			std::string hash = getParam("m");
			std::string hashesPath = Paths::getGlobalAchievementsPath() + "/hashes.json";
			int gameId = 0;
			if (Utils::FileSystem::exists(hashesPath)) {
				std::string json = Utils::FileSystem::readAllText(hashesPath);
				rapidjson::Document doc;
				doc.Parse(json.c_str());
				if (!doc.HasParseError() && doc.HasMember(hash.c_str())) {
					gameId = doc[hash.c_str()].GetInt();
				}
			}
			if (gameId != 0) {
				std::string patchPath = Paths::getGlobalAchievementsPath() + "/patchdata/" + std::to_string(gameId) + ".json";
				if (Utils::FileSystem::exists(patchPath)) {
					std::string jsonStr = Utils::FileSystem::readAllText(patchPath);
					rapidjson::Document patchDoc;
					patchDoc.Parse(jsonStr.c_str());
					if (!patchDoc.HasParseError() && patchDoc.HasMember("PatchData")) {
						const rapidjson::Value& pd = patchDoc["PatchData"];
						rapidjson::Document v2Doc;
						v2Doc.SetObject();
						rapidjson::Document::AllocatorType& allocator = v2Doc.GetAllocator();
						
						v2Doc.AddMember("Success", true, allocator);
						if (pd.HasMember("ID")) v2Doc.AddMember("GameId", rapidjson::Value(pd["ID"], allocator), allocator);
						if (pd.HasMember("Title")) v2Doc.AddMember("Title", rapidjson::Value(pd["Title"], allocator), allocator);
						if (pd.HasMember("ConsoleID")) v2Doc.AddMember("ConsoleId", rapidjson::Value(pd["ConsoleID"], allocator), allocator);
						
						rapidjson::Value imageIconValue;
						if (pd.HasMember("ImageIcon") && pd["ImageIcon"].IsString()) {
							std::string iconStr = pd["ImageIcon"].GetString();
							if (iconStr.find("http") != 0) iconStr = "https://retroachievements.org" + iconStr;
							imageIconValue.SetString(iconStr.c_str(), allocator);
						} else {
							imageIconValue.SetString("", allocator);
						}
						v2Doc.AddMember("ImageIconUrl", imageIconValue, allocator);
						
						if (pd.HasMember("ID")) v2Doc.AddMember("RichPresenceGameId", rapidjson::Value(pd["ID"], allocator), allocator);
						v2Doc.AddMember("RichPresencePatch", "", allocator);
						
						rapidjson::Value setsArray(rapidjson::kArrayType);
						rapidjson::Value setObj(rapidjson::kObjectType);
						setObj.AddMember("AchievementSetId", 1, allocator);
						if (pd.HasMember("ID")) setObj.AddMember("GameId", rapidjson::Value(pd["ID"], allocator), allocator);
						setObj.AddMember("Title", rapidjson::Value(rapidjson::kNullType), allocator);
						setObj.AddMember("Type", "core", allocator);
						
						rapidjson::Value setIconValue;
						setIconValue.CopyFrom(v2Doc["ImageIconUrl"], allocator);
						setObj.AddMember("ImageIconUrl", setIconValue, allocator);
						
						if (pd.HasMember("Achievements")) {
							rapidjson::Value achArray(rapidjson::kArrayType);
							achArray.CopyFrom(pd["Achievements"], allocator);
							setObj.AddMember("Achievements", achArray, allocator);
						} else {
							setObj.AddMember("Achievements", rapidjson::Value(rapidjson::kArrayType), allocator);
						}
						
						if (pd.HasMember("Leaderboards")) {
							rapidjson::Value lbArray(rapidjson::kArrayType);
							lbArray.CopyFrom(pd["Leaderboards"], allocator);
							setObj.AddMember("Leaderboards", lbArray, allocator);
						} else {
							setObj.AddMember("Leaderboards", rapidjson::Value(rapidjson::kArrayType), allocator);
						}
						
						setsArray.PushBack(setObj, allocator);
						v2Doc.AddMember("Sets", setsArray, allocator);
						
						rapidjson::StringBuffer buffer;
						rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
						v2Doc.Accept(writer);
						
						res.set_content(buffer.GetString(), "application/json");
						return;
					}
					res.set_content(jsonStr, "application/json");
					return;
				}
			}
			res.status = 404;
		}
		else if (r == "startsession") {
			res.set_content("{\"Success\":true,\"ServerNow\":" + std::to_string(time(nullptr)) + ",\"Unlocks\":[],\"HardcoreUnlocks\":[]}", "application/json");
		}
		else if (r == "login" || r == "login2") {
			std::string u = getParam("u");
			if (u.empty()) u = "offline";
			std::string jsonStr = "{\"Success\":true,\"User\":\"" + u + "\",\"Token\":\"offline_token\",\"Score\":0,\"SoftcoreScore\":0,\"DisplayName\":\"" + u + "\"}";
			res.set_content(jsonStr, "application/json");
		}
		else if (r == "awardachievement") {
			std::string a = getParam("a");
			int gameId = -1;
			std::string gamesDir = "/roms/achievements/games";
			auto files = Utils::FileSystem::getDirContent(gamesDir);
			for (auto& file : files) {
				std::string json = Utils::FileSystem::readAllText(file);
				if (json.find("\"ID\":" + a + ",") != std::string::npos || json.find("\"ID\":\"" + a + "\"") != std::string::npos) {
					rapidjson::Document doc;
					doc.Parse(json.c_str());
					if (!doc.HasParseError() && doc.HasMember("Achievements")) {
						const auto& achs = doc["Achievements"];
						for (auto it = achs.MemberBegin(); it != achs.MemberEnd(); ++it) {
							std::string achId;
							if (it->value["ID"].IsInt()) achId = std::to_string(it->value["ID"].GetInt());
							else achId = it->value["ID"].GetString();
							
							if (achId == a) {
								if (doc.HasMember("ID")) {
									gameId = doc["ID"].GetInt();
									break;
								}
							}
						}
					}
				}
				if (gameId != -1) break;
			}
			
			if (gameId != -1) {
				GameInfoAndUserProgress prog;
				AchievementCache::loadGameData(gameId, prog);
				AchievementCache::loadUserProgress(gameId, prog);
				
				bool hardcore = getParam("h") == "1";
				
				for (auto& ach : prog.Achievements) {
					if (ach.ID == a) {
						if (ach.DateEarned.empty()) {
							ach.DateEarned = "offline";
						}
						if (hardcore && ach.DateEarnedHardcore.empty()) {
							ach.DateEarnedHardcore = "offline";
						}
						break;
					}
				}
				
				AchievementCache::saveUserProgress(gameId, prog);
				res.set_content("{\"Success\":true,\"Score\":10,\"SoftcoreScore\":10,\"AchievementID\":" + a + ",\"AchievementsRemaining\":0}", "application/json");
			} else {
				res.status = 404;
			}
		}
	};

	sServer->Get("/dorequest.php", handler);
	sServer->Post("/dorequest.php", handler);

	sServer->Get(R"(/Badge/(.*))", [](const httplib::Request& req, httplib::Response& res) {
		std::string name = req.matches[1];
		std::string localPath = Paths::getGlobalAchievementsPath() + "/badges/" + name;
		LOG(LogDebug) << "AchievementServer Badge request: " << name;
		if (Utils::FileSystem::exists(localPath)) {
			res.set_content(Utils::FileSystem::readAllText(localPath), "image/png");
		} else {
			res.status = 404;
		}
	});

	sRunning = true;
	sThread = std::thread([]() {
		sServer->listen("127.0.0.1", 9191);
	});
}

void AchievementServer::stop()
{
	if (sRunning) {
		sServer->stop();
		sThread.join();
		sServer.reset();
		sRunning = false;
	}
}
