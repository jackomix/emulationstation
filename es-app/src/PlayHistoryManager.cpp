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

std::string PlayHistoryManager::getTempSessionFilePath() {
    std::string profilePath = ProfileManager::getInstance()->getProfileDataPath();
    std::string historyDir = profilePath + "/playhistory";
    if (!Utils::FileSystem::exists(historyDir))
        Utils::FileSystem::createDirectory(historyDir);
    return historyDir + "/active_session.json";
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
            } else {
                Utils::FileSystem::writeAllText(path + ".corrupted", content);
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
    
    rename(tmpPath.c_str(), path.c_str());
}

void PlayHistoryManager::saveTempSession() {
    if (mCurrentGame == nullptr) return;
    
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    doc.AddMember("historyFilePath", rapidjson::Value(getHistoryFilePath(mCurrentGame).c_str(), allocator), allocator);
    doc.AddMember("gamePath", rapidjson::Value(mCurrentGame->getPath().c_str(), allocator), allocator);
    doc.AddMember("id", rapidjson::Value(mCurrentSession.id.c_str(), allocator), allocator);
    doc.AddMember("startTime", rapidjson::Value(mCurrentSession.startTime.c_str(), allocator), allocator);
    doc.AddMember("durationSeconds", mCurrentSession.durationSeconds, allocator);
    doc.AddMember("completed", mCurrentSession.completed, allocator);
    
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto& a : mCurrentSession.achievementIds) {
        arr.PushBack(rapidjson::Value(a.c_str(), allocator), allocator);
    }
    doc.AddMember("achievementIds", arr, allocator);
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    Utils::FileSystem::writeAllText(getTempSessionFilePath(), buffer.GetString());
}

void PlayHistoryManager::mergeTempSession() {
    std::string path = getTempSessionFilePath();
    if (!Utils::FileSystem::exists(path)) return;
    
    std::string content = Utils::FileSystem::readAllText(path);
    if (content.empty()) {
        Utils::FileSystem::removeFile(path);
        return;
    }
    
    rapidjson::Document doc;
    doc.Parse(content.c_str());
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("historyFilePath") || !doc.HasMember("gamePath")) {
        Utils::FileSystem::removeFile(path);
        return;
    }
    
    std::string historyFilePath = doc["historyFilePath"].GetString();
    std::string gamePath = doc["gamePath"].GetString();
    
    PlaySession s;
    if (doc.HasMember("id") && doc["id"].IsString()) s.id = doc["id"].GetString();
    if (doc.HasMember("startTime") && doc["startTime"].IsString()) s.startTime = doc["startTime"].GetString();
    if (doc.HasMember("durationSeconds") && doc["durationSeconds"].IsInt()) s.durationSeconds = doc["durationSeconds"].GetInt();
    s.completed = true;
    if (doc.HasMember("achievementIds") && doc["achievementIds"].IsArray()) {
        for (auto& a : doc["achievementIds"].GetArray()) {
            if (a.IsString()) s.achievementIds.push_back(a.GetString());
        }
    }
    
    rapidjson::Document historyDoc;
    historyDoc.SetObject();
    
    if (Utils::FileSystem::exists(historyFilePath)) {
        std::string histContent = Utils::FileSystem::readAllText(historyFilePath);
        if (!histContent.empty()) {
            rapidjson::Document existing;
            existing.Parse(histContent.c_str());
            if (!existing.HasParseError() && existing.IsObject()) {
                historyDoc.CopyFrom(existing, historyDoc.GetAllocator());
            }
        }
    }
    
    rapidjson::Document::AllocatorType& allocator = historyDoc.GetAllocator();
    
    rapidjson::Value sessionsArray(rapidjson::kArrayType);
    if (historyDoc.HasMember(gamePath.c_str()) && historyDoc[gamePath.c_str()].IsArray()) {
        for (auto& v : historyDoc[gamePath.c_str()].GetArray()) {
            std::string sid = "";
            if (v.HasMember("id") && v["id"].IsString()) sid = v["id"].GetString();
            if (sid != s.id) {
                rapidjson::Value sObj(rapidjson::kObjectType);
                sObj.CopyFrom(v, allocator);
                sessionsArray.PushBack(sObj, allocator);
            }
        }
    }
    
    rapidjson::Value newSObj(rapidjson::kObjectType);
    newSObj.AddMember("id", rapidjson::Value(s.id.c_str(), allocator), allocator);
    newSObj.AddMember("startTime", rapidjson::Value(s.startTime.c_str(), allocator), allocator);
    newSObj.AddMember("durationSeconds", s.durationSeconds, allocator);
    newSObj.AddMember("completed", s.completed, allocator);
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto& a : s.achievementIds) {
        arr.PushBack(rapidjson::Value(a.c_str(), allocator), allocator);
    }
    newSObj.AddMember("achievementIds", arr, allocator);
    
    sessionsArray.PushBack(newSObj, allocator);
    
    if (historyDoc.HasMember(gamePath.c_str())) {
        historyDoc.RemoveMember(gamePath.c_str());
    }
    historyDoc.AddMember(rapidjson::Value(gamePath.c_str(), allocator), sessionsArray, allocator);
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    historyDoc.Accept(writer);
    
    std::string tmpPath = historyFilePath + ".tmp";
    Utils::FileSystem::writeAllText(tmpPath, buffer.GetString());
    rename(tmpPath.c_str(), historyFilePath.c_str());
    
    Utils::FileSystem::removeFile(path);
}

void PlayHistoryManager::startSession(FileData* game) {
    if (game == nullptr) return;
    
    {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        mRunHeartbeat = false;
        mHeartbeatCV.notify_all();
    }
    if (mHeartbeatThread.joinable()) {
        mHeartbeatThread.join();
    }
    
    mergeTempSession();
    
    std::lock_guard<std::mutex> lock(mSessionMutex);
    
    mCurrentGame = game;
    time_t rawtime = Utils::Time::now();
    mCurrentSession.id = std::to_string(rawtime);
    
    mCurrentSession.startTime = Utils::Time::timeToString(rawtime, "%Y-%m-%dT%H:%M:%S");
    
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
            
            mCurrentGame->setMetadata(MetaDataId::LastPlayed, Utils::Time::DateTime(Utils::Time::now()));
        }
    }
    
    sessions.push_back(mCurrentSession);
    saveSessions(mCurrentGame, sessions);
    saveTempSession();
    
    mRunHeartbeat = true;
    mHeartbeatThread = std::thread(&PlayHistoryManager::heartbeatLoop, this);
}

void PlayHistoryManager::stopSession() {
    {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        if (!mRunHeartbeat) return;
        mRunHeartbeat = false;
        mHeartbeatCV.notify_all();
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
    
    saveTempSession();
    mergeTempSession();
    
    mCurrentGame = nullptr;
}

void PlayHistoryManager::heartbeatLoop() {
    while (mRunHeartbeat) {
        {
            std::unique_lock<std::mutex> cvLock(mHeartbeatMutex);
            if (mHeartbeatCV.wait_for(cvLock, std::chrono::seconds(30), [this]() { return !mRunHeartbeat; })) {
                break;
            }
        }
        
        std::lock_guard<std::mutex> lock(mSessionMutex);
        if (mCurrentGame == nullptr) continue;
        
        mCurrentSession.durationSeconds += 30;
        saveTempSession();
    }
}

void PlayHistoryManager::updateAchievementsForGame(FileData* game, const GameInfoAndUserProgress& raInfo) {
    // Rely on direct calls now.
}

void PlayHistoryManager::addAchievementToCurrentSession(const std::string& achId) {
    std::lock_guard<std::mutex> lock(mSessionMutex);
    if (mCurrentGame == nullptr) return;
    
    if (std::find(mCurrentSession.achievementIds.begin(), mCurrentSession.achievementIds.end(), achId) == mCurrentSession.achievementIds.end()) {
        mCurrentSession.achievementIds.push_back(achId);
        saveTempSession();
    }
}
