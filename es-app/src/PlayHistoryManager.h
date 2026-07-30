#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

struct PlaySession {
    std::string id;
    std::string startTime;
    int durationSeconds;
    bool completed;
    std::vector<std::string> achievementIds;
};

class FileData;
struct GameInfoAndUserProgress;

class PlayHistoryManager {
public:
    static PlayHistoryManager* getInstance();
    
    void startSession(FileData* game);
    void stopSession();
    PlaySession getLastSession() const { return mLastSession; }
    
    std::vector<PlaySession> getSessions(FileData* game);
    void saveSessions(FileData* game, const std::vector<PlaySession>& sessions);
    void updateAchievementsForGame(FileData* game, const GameInfoAndUserProgress& raInfo);
    void addAchievementToCurrentSession(const std::string& achId);

private:
    PlayHistoryManager();
    static PlayHistoryManager* sInstance;
    
    std::string getHistoryFilePath(FileData* game);
    std::string getTempSessionFilePath();
    void saveTempSession();
    void mergeTempSession();
    
    std::atomic<bool> mRunHeartbeat;
    std::thread mHeartbeatThread;
    std::condition_variable mHeartbeatCV;
    std::mutex mHeartbeatMutex;
    
    std::mutex mSessionMutex;
    PlaySession mCurrentSession;
    PlaySession mLastSession;
    FileData* mCurrentGame = nullptr;
    
    void heartbeatLoop();
};
