#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

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
    
    std::vector<PlaySession> getSessions(FileData* game);
    void saveSessions(FileData* game, const std::vector<PlaySession>& sessions);
    void updateAchievementsForGame(FileData* game, const GameInfoAndUserProgress& raInfo);

private:
    PlayHistoryManager();
    static PlayHistoryManager* sInstance;
    
    std::string getHistoryFilePath(FileData* game);
    
    std::atomic<bool> mRunHeartbeat;
    std::thread mHeartbeatThread;
    
    std::mutex mSessionMutex;
    PlaySession mCurrentSession;
    FileData* mCurrentGame = nullptr;
    
    void heartbeatLoop();
};
