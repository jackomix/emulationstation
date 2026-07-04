#include "AchievementServer.h"
#include "services/httplib.h"
#include "Paths.h"
#include "utils/FileSystemUtil.h"
#include "AchievementCache.h"
#include "RetroAchievements.h"
#include <thread>
#include <memory>
#include <rapidjson/document.h>

static std::unique_ptr<httplib::Server> sServer;
static std::thread sThread;
static bool sRunning = false;

void AchievementServer::start()
{
	if (sRunning) return;
	
	sServer = std::make_unique<httplib::Server>();
	
	auto handler = [](const httplib::Request& req, httplib::Response& res) {
		if (!req.has_param("r")) {
			res.status = 400;
			return;
		}
		std::string r = req.get_param_value("r");
		
		if (r == "gameid") {
			std::string hash = req.get_param_value("m");
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
			res.set_content("{\"GameID\":" + std::to_string(gameId) + "}", "application/json");
		}
		else if (r == "patch") {
			std::string g = req.get_param_value("g");
			std::string patchPath = Paths::getGlobalAchievementsPath() + "/patchdata/" + g + ".json";
			if (Utils::FileSystem::exists(patchPath)) {
				res.set_content(Utils::FileSystem::readAllText(patchPath), "application/json");
			} else {
				res.status = 404;
			}
		}
		else if (r == "startsession") {
			res.set_content("{\"Success\":true}", "application/json");
		}
		else if (r == "login") {
			std::string u = req.get_param_value("u");
			if (u.empty()) u = "offline";
			std::string jsonStr = "{\"Success\":true,\"User\":\"" + u + "\",\"Token\":\"offline_token\",\"Score\":0,\"SoftcoreScore\":0,\"DisplayName\":\"" + u + "\"}";
			res.set_content(jsonStr, "application/json");
		}
		else if (r == "awardachievement") {
			std::string a = req.get_param_value("a");
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
				
				bool hardcore = req.has_param("h") && req.get_param_value("h") == "1";
				
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
				res.set_content("{\"Success\":true,\"Score\":10}", "application/json");
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
