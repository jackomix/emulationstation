using LAHEE.Data;
using RATools.Data;
using Newtonsoft.Json;
using CodeNote = LAHEE.Data.CodeNote;

namespace LAHEE;

class RAAnyResponse {
    public bool Success;
}

class RAErrorResponse : RAAnyResponse {
    public string Error;
    public int Code;

    public RAErrorResponse(string error) {
        Error = error;
    }
}

class RALoginResponse : RAAnyResponse {
    public string User;
    public string Token;
    public int Score;
    public int SoftcoreScore;
    public int Messages;
    public int Permissions;
    public string AccountType;
    public string DisplayName;
}

class RAGameIDResponse : RAAnyResponse {
    public uint GameID;
}

class RAApiResolveHashResponse : RAAnyResponse {
    public uint GameID;
}

class RAApiGameResponse : RAAnyResponse {
    public uint ID;
    public string Title;
    public string ImageIcon;
}

class RAPatchResponse : RAAnyResponse {
    public GameData PatchData;
}

// Model for r=achievementsets (Requires CamelCase Id/Url)
class RAAchievementSetsResponse : RAAnyResponse {
    public uint GameId;
    public String Title;
    public String ImageIconUrl;
    public uint RichPresenceGameId;
    public String RichPresencePatch;
    public int ConsoleId;
    public List<SetData> Sets;
}

// Model for r=patch (Requires All-Caps ID/URL)
class RAPatchResponseV2 : RAAnyResponse {
    [JsonProperty("GameID")]
    public uint GameID;
    public String Title;
    [JsonProperty("ImageIconURL")]
    public String ImageIconURL;
    [JsonProperty("RichPresenceGameID")]
    public uint RichPresenceGameID;
    public String RichPresencePatch;
    [JsonProperty("ConsoleID")]
    public int ConsoleID;
    public List<SetData> Sets;
}

class RAStartSessionResponse : RAAnyResponse {
    public RAStartSessionAchievementData[] Unlocks;
    public RAStartSessionAchievementData[] HardcoreUnlocks;
    public long ServerNow;

    public class RAStartSessionAchievementData {
        public int ID;
        public long When;

        public RAStartSessionAchievementData() {
        }

        public RAStartSessionAchievementData(UserAchievementData userAchievement, bool isHardcore) {
            ID = userAchievement.AchievementID;
            When = isHardcore ? userAchievement.AchieveDate : userAchievement.AchieveDateSoftcore;
        }
    }
}

class RAAchievementListResponse : RAAnyResponse {
    public int[] UserUnlocks;
    [JsonProperty("GameID")]
    public int GameID;
    public bool HardcoreMode;
}

class RAUnlockResponse : RAAnyResponse {
    public int Score;
    public int SoftcoreScore;
    public int AchievementID;
    public int AchievementsRemaining;
}

class RALeaderboardResponse : RAAnyResponse {
    public RALeaderboardResponseV2 Response;
    public int Score;
    public int BestScore;
    public RankObject RankInfo;
    public TopObject[] TopEntries;

    public class RankObject {
        public int Rank;
        public int NumEntries;
    }

    public class TopObject {
        public String User;
        public int Rank;
        public int Score;
    }
}

class RALeaderboardResponseV2 : RAAnyResponse {
    public LeaderboardData LBData;
    public int Score;
    public String ScoreFormatted;
    public int BestScore;
    public RALeaderboardResponse.RankObject RankInfo;
    public RALeaderboardResponse.TopObject[] TopEntries;
    public RALeaderboardResponse.TopObject[] TopEntriesFriends;
}

class LaheeResponse {
    public String version;
    public UserData[] users;
    public GameData[] games;
    public UserComment[] comments;
    public string[] notifications;
    public Dictionary<int, AchievementExtendedData> achievements_extended;
}

class LaheeUserResponse {
    public uint current_game_id;
    public DateTime? last_ping;
    public DateTime? last_play;
    public TimeSpan? play_time;
    public String game_status;
    public Dictionary<int, UserAchievementData> achievements;
}

class RAApiHashesResponse {
    public Hash[] Results;

    internal class Hash {
        public String MD5;
        public String Name;
        public String[] Labels;
        public String PatchUrl;
    }
}

class RAApiCommentsResponse {
    public int Count;
    public int Total;
    public UserComment[] Results;
}

class LaheeFetchCommentsResponse : RAAnyResponse {
    public UserComment[] Comments;
}

class LaheeWriteCommentResponse : LaheeFetchCommentsResponse {
}

class LaheeFlagImportantResponse : RAAnyResponse {
    public List<int> Flagged;
}

class RALatestIntegrationResponse : RAAnyResponse {
    public String MinimumVersion;
    public String LatestVersion;
    public String LatestVersionUrl;
    public String LatestVersionUrlX64;
}

class RACodeNotesResponse : RAAnyResponse {
    public List<CodeNote> CodeNotes;
}

class RABadgeIterResponse : RAAnyResponse {
    public int FirstBadge;
    public int NextBadge;
}

class RAUploadAchievementResponse : RAAnyResponse {
    public int AchievementID;
    public String Error;
}

class RAUploadFileResponse : RAAnyResponse {
    public ResponseClass Response;

    public class ResponseClass {
        public String BadgeIter;
    }
}

class RARichPresencePatchResponse : RAAnyResponse {
    public String RichPresencePatch;
}

class LaheeAchievementCodeResponse : RAAnyResponse {
    public List<CodeNote> CodeNotes;
    public RequirementGroup[] TriggerGroups;
}

class RAApiGameExtendedResponse {
    public Dictionary<uint, ServerAchievementData> Achievements;

    public class ServerAchievementData {
        public int ID;
        public String Title;
        public String MemAddr;
        // remaining fields omitted

        public override string ToString() {
            return Title + " (" + ID + ")";
        }
    }
}

class RAHashLibraryResponse : RAAnyResponse {
    public Dictionary<String, UInt32> MD5List;
}

class RAAllProgressResponse : RAAnyResponse {
    public Dictionary<UInt32, Progress> Response;

    public class Progress {
        public int Achievements;
    }
}

class RAGameInfoListResponse : RAAnyResponse {
    public GameData[] Response;
}
