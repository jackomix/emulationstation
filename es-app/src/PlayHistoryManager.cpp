#include "PlayHistoryManager.h"
#include "FileData.h"
#include "SystemData.h"
#include "ProfileManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/TimeUtil.h"
#include "utils/StringUtil.h"
#include "RetroAchievements.h"
#include "Log.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <chrono>

PlayHistoryManager* PlayHistoryManager::sInstance = nullptr;

PlayHistoryManager* PlayHistoryManager::getInstance() {
    if (sInstance == nullptr)
        sInstance = new PlayHistoryManager();
    return sInstance;
}

PlayHistoryManager::PlayHistoryManager() : mRunHeartbeat(false) {
}

std::string PlayHistoryManager::getHistoryFilePath(FileData* game) {
    std::string profilePath = ProfileManager::getInstance()->getProfileDataPath();
    std::string historyDir = profilePath + "/playhistory";
    if (!Utils::FileSystem::exists(historyDir))
        Utils::FileSystem::createDirectory(historyDir);
    
    std::string sysName = game->getSourceFileData()->getSystem()->getName();
    return historyDir + "/" + sysName + "_history.json";
}

std::vector<PlaySession> PlayHistoryManager::getSessions(FileData* game) {
    std::vector<PlaySession> sessions;
    if (game == nullptr) return sessions;
    
    std::string path = getHistoryFilePath(game);
    if (!Utils::FileSystem::exists(path)) return sessions;
    
    std::string content = Utils::FileSystem::readAllText(path);
    if (content.empty()) return sessions;
    
    rapidjson::Document doc;
    doc.Parse(content.c_str());
    if (doc.HasParseError() || !doc.IsObject()) return sessions;
    
    std::string gamePath = game->getPath();
    if (doc.HasMember(gamePath.c_str()) && doc[gamePath.c_str()].IsArray()) {
        for (auto& v : doc[gamePath.c_str()].GetArray()) {
            PlaySession s;
            if (v.HasMember("id") && v["id"].IsString()) s.id = v["id"].GetString();
            if (v.HasMember("startTime") && v["startTime"].IsString()) s.startTime = v["startTime"].GetString();
            if (v.HasMember("durationSeconds") && v["durationSeconds"].IsInt()) s.durationSeconds = v["durationSeconds"].GetInt();
            if (v.HasMember("completed") && v["completed"].IsBool()) s.completed = v["completed"].GetBool();
            
            if (v.HasMember("achievementIds") && v["achievementIds"].IsArray()) {
                for (auto& a : v["achievementIds"].GetArray()) {
                    if (a.IsString()) s.achievementIds.push_back(a.GetString());
                }
            }
            sessions.push_back(s);
        }
    }
    
    return sessions;
}

void PlayHistoryManager::saveSessions(FileData* game, const std::vector<PlaySession>& sessions) {
    if (game == nullptr) return;
    std::string path = getHistoryFilePath(game);
    
    rapidjson::Document doc;
    doc.SetObject();
    
    if (Utils::FileSystem::exists(path)) {
        std::string content = Utils::FileSystem::readAllText(path);
        if (!content.empty()) {
            rapidjson::Document existing;
            existing.Parse(content.c_str());
            if (!existing.HasParseError() && existing.IsObject()) {
                doc.CopyFrom(existing, doc.GetAllocator());
            }
        }
    }
    
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    std::string gamePath = game->getPath();
    
    rapidjson::Value sessionsArray(rapidjson::kArrayType);
    for (const auto& s : sessions) {
        rapidjson::Value sObj(rapidjson::kObjectType);
        sObj.AddMember("id", rapidjson::Value(s.id.c_str(), allocator), allocator);
        sObj.AddMember("startTime", rapidjson::Value(s.startTime.c_str(), allocator), allocator);
        sObj.AddMember("durationSeconds", s.durationSeconds, allocator);
        sObj.AddMember("completed", s.completed, allocator);
        
        rapidjson::Value arr(rapidjson::kArrayType);
        for (const auto& a : s.achievementIds) {
            arr.PushBack(rapidjson::Value(a.c_str(), allocator), allocator);
        }
        sObj.AddMember("achievementIds", arr, allocator);
        
        sessionsArray.PushBack(sObj, allocator);
    }
    
    if (doc.HasMember(gamePath.c_str())) {
        doc.RemoveMember(gamePath.c_str());
    }
    doc.AddMember(rapidjson::Value(gamePath.c_str(), allocator), sessionsArray, allocator);
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    std::string tmpPath = path + ".tmp";
    Utils::FileSystem::writeAllText(tmpPath, buffer.GetString());
    
    remove(path.c_str());
    rename(tmpPath.c_str(), path.c_str());
}

void PlayHistoryManager::startSession(FileData* game) {
    if (game == nullptr) return;
    
    std::lock_guard<std::mutex> lock(mSessionMutex);
    
    mCurrentGame = game;
    time_t rawtime = Utils::Time::now();
    mCurrentSession.id = std::to_string(rawtime);
    
    struct tm * ptminfo = gmtime(&rawtime);
    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", ptminfo);
    mCurrentSession.startTime = std::string(buf);
    
    mCurrentSession.durationSeconds = 0;
    mCurrentSession.completed = false;
    mCurrentSession.achievementIds.clear();
    
    auto sessions = getSessions(mCurrentGame);
    bool needsSave = false;
    for (auto& s : sessions) {
        if (!s.completed) {
            s.completed = true;
            needsSave = true;
            int playCount = mCurrentGame->getMetadata().getInt(MetaDataId::PlayCount) + 1;
            mCurrentGame->setMetadata(MetaDataId::PlayCount, std::to_string(static_cast<long long>(playCount)));
            long gameTime = mCurrentGame->getMetadata().getInt(MetaDataId::GameTime) + s.durationSeconds;
            if (s.durationSeconds >= 10)
                mCurrentGame->setMetadata(MetaDataId::GameTime, std::to_string(static_cast<long>(gameTime)));
            
            // For LastPlayed, just use now since we are booting/recovering
            mCurrentGame->setMetadata(MetaDataId::LastPlayed, Utils::Time::DateTime(Utils::Time::now()));
        }
    }
    
    sessions.push_back(mCurrentSession);
    saveSessions(mCurrentGame, sessions);
    
    mRunHeartbeat = true;
    mHeartbeatThread = std::thread(&PlayHistoryManager::heartbeatLoop, this);
}

void PlayHistoryManager::stopSession() {
    {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        if (!mRunHeartbeat) return;
        mRunHeartbeat = false;
    }
    
    if (mHeartbeatThread.joinable()) {
        mHeartbeatThread.join();
    }
    
    std::lock_guard<std::mutex> lock(mSessionMutex);
    if (mCurrentGame == nullptr) return;
    
    time_t rawtime = Utils::Time::now();
    try {
        mCurrentSession.durationSeconds = (int)(rawtime - std::stoll(mCurrentSession.id));
    } catch (...) {}
    
    mCurrentSession.completed = true;
    
    auto sessions = getSessions(mCurrentGame);
    for (auto& s : sessions) {
        if (s.id == mCurrentSession.id) {
            s.durationSeconds = mCurrentSession.durationSeconds;
            s.completed = true;
            break;
        }
    }
    
    saveSessions(mCurrentGame, sessions);
    mCurrentGame = nullptr;
}

void PlayHistoryManager::heartbeatLoop() {
    int ticks = 0;
    while (mRunHeartbeat) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!mRunHeartbeat) break;
        
        ticks++;
        if (ticks >= 30) {
            ticks = 0;
            std::lock_guard<std::mutex> lock(mSessionMutex);
            if (mCurrentGame == nullptr) continue;
            
            mCurrentSession.durationSeconds += 30;
            auto sessions = getSessions(mCurrentGame);
            for (auto& s : sessions) {
                if (s.id == mCurrentSession.id) {
                    s.durationSeconds = mCurrentSession.durationSeconds;
                    break;
                }
            }
            saveSessions(mCurrentGame, sessions);
        }
    }
}

void PlayHistoryManager::updateAchievementsForCurrentSession(const GameInfoAndUserProgress& raInfo) {
    std::lock_guard<std::mutex> lock(mSessionMutex);
    if (mCurrentGame == nullptr) return;
    if (!mCurrentSession.completed) return;
    if (raInfo.Achievements.empty()) return;

    long sStart = std::stoll(mCurrentSession.id);
    long sEnd = sStart + mCurrentSession.durationSeconds + 60; // 60s buffer
    
    struct tm * ptminfoStart = gmtime(&sStart);
    char bufStart[128];
    strftime(bufStart, sizeof(bufStart), "%Y-%m-%dT%H:%M:%SZ", ptminfoStart);
    std::string sIso = bufStart;
    
    struct tm * ptminfoEnd = gmtime(&sEnd);
    char bufEnd[128];
    strftime(bufEnd, sizeof(bufEnd), "%Y-%m-%dT%H:%M:%SZ", ptminfoEnd);
    std::string eIso = bufEnd;
    
    bool updated = false;
    for (auto& ach : raInfo.Achievements) {
        std::string earned = ach.DateEarned.empty() ? ach.DateEarnedHardcore : ach.DateEarned;
        if (earned.empty()) continue;
        
        std::string earnedIso = Utils::String::replace(earned, " ", "T") + "Z";
        
        if (earnedIso >= sIso && earnedIso <= eIso) {
            if (std::find(mCurrentSession.achievementIds.begin(), mCurrentSession.achievementIds.end(), ach.ID) == mCurrentSession.achievementIds.end()) {
                mCurrentSession.achievementIds.push_back(ach.ID);
                updated = true;
            }
        }
    }
    
    if (updated) {
        auto sessions = getSessions(mCurrentGame);
        for (auto& s : sessions) {
            if (s.id == mCurrentSession.id) {
                s.achievementIds = mCurrentSession.achievementIds;
                break;
            }
        }
        saveSessions(mCurrentGame, sessions);
    }
}

