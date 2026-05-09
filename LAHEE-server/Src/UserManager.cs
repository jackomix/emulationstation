using LAHEE.Data;
using LAHEE.Util;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using WatsonWebserver.Core;

namespace LAHEE;

class UserManager {
    public static string UserDataDirectory { get; set; }
    public static UserData ActiveUser { get; set; }
    public static string ActiveProfileId { get; set; } = "1";

    private static Dictionary<string, UserData> userData;
    private static Dictionary<string, UserData> activeTokens;
    private static readonly Lock SAVE_LOCK = new Lock();

    internal static void Initialize() {
        activeTokens = new Dictionary<string, UserData>();

        // 1. Resolve Master Pointer (Source of Truth)
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        string statePath = Path.Combine(hubDir, "active_profile.json");
        string activeId = "1"; // Safe Default

        if (File.Exists(statePath)) {
            try {
                var state = JsonConvert.DeserializeObject<Dictionary<string, string>>(File.ReadAllText(statePath));
                if (state != null && state.ContainsKey("active_id")) {
                    string val = state["active_id"];
                    // SAFETY: Only allow alphanumeric IDs to prevent path traversal
                    if (!string.IsNullOrEmpty(val) && val.All(char.IsLetterOrDigit)) {
                        activeId = val;
                    }
                }
            } catch (Exception ex) {
                Log.User.LogWarning("Master pointer corrupt, defaulting to ID 1: {e}", ex.Message);
            }
        }

        ActiveProfileId = activeId;

        // 2. Set Path to ID Folder
        string romsRoot = Path.GetDirectoryName(hubDir);
        if (string.IsNullOrEmpty(romsRoot)) romsRoot = ".."; // Fallback for relative setups
        
        string targetDir = Path.Combine(romsRoot, "Profiles", activeId);

        // 3. Folder Safety: If the pointer says "ID 5" but folder 5 is gone, fallback to 1
        if (!Directory.Exists(targetDir) && activeId != "1") {
            Log.User.LogWarning("Profile folder {id} not found, falling back to ID 1", activeId);
            activeId = "1";
            ActiveProfileId = "1";
            targetDir = Path.Combine(romsRoot, "Profiles", "1");
        }

        UserDataDirectory = targetDir;

        // 4. Load from static filename
        Load(UserDataDirectory);

        if (userData.Count == 0) {
            Log.User.LogInformation("Profile {id} is empty. Initializing Player...", activeId);
            RegisterNewUser("Player"); 
            Save();
        }

        // Whoever is in this private folder IS the active user
        ActiveUser = userData.Values.FirstOrDefault();

        Log.User.LogInformation("ID Master initialized: Profile {id} ({u})", activeId, ActiveUser?.UserName);
    }

    public static void SaveActiveUser() {
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        if (string.IsNullOrEmpty(hubDir)) return;

        string statePath = Path.Combine(hubDir, "active_profile.json");
        var state = new Dictionary<string, string> {
            { "active_id", ActiveProfileId }
        };
        File.WriteAllText(statePath, JsonConvert.SerializeObject(state, Formatting.Indented));
    }

    public static void Load(string dir) {
        userData = new Dictionary<string, UserData>();

        if (!Directory.Exists(dir)) {
            Directory.CreateDirectory(dir);
            return;
        }

        string achFile = Path.Combine(dir, "achievements.json");
        if (File.Exists(achFile)) {
            try {
                UserData data = JsonConvert.DeserializeObject<UserData>(File.ReadAllText(achFile));
                Migrate(data);
                userData[data.UserName.ToLower()] = data;
                Log.User.LogDebug("Loaded achievements from {f}", achFile);
            } catch (Exception ex) {
                Log.User.LogError(ex, "Failed to load achievements from {f}", achFile);
            }
        }
    }

    private static void Migrate(UserData data) {
        if (data.GameData == null) data.GameData = new Dictionary<uint, UserGameData>();
        foreach (UserGameData ugd in data.GameData.Values) {
            if (ugd.FlaggedAchievements == null) {
                ugd.FlaggedAchievements = new List<int>();
            }
        }
    }

    public static UserData GetUserData(string username) {
        if (string.IsNullOrEmpty(username)) return null;
        string key = username.ToLower();
        if (userData.ContainsKey(key)) {
            return userData[key];
        } else {
            // In ID Master, we might need to find the user in the ONLY loaded file
            return userData.Values.FirstOrDefault(u => u.UserName.Equals(username, StringComparison.OrdinalIgnoreCase));
        }
    }

    public static UserData RegisterNewUser(string username) {
        UserData user = new UserData() {
            AllowUse = true,
            UserName = username,
            ID = new Random().Next(),
            GameData = new Dictionary<uint, UserGameData>()
        };
        userData[username.ToLower()] = user;
        return user;
    }

    public static void Save() {
        Save(UserDataDirectory);
    }

    public static void Save(string dir) {
        if (ActiveUser == null) return;

        lock (SAVE_LOCK) {
            if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
            
            string outputFile = Path.Combine(dir, "achievements.json");
            string backupFile = Path.Combine(dir, "achievements.bak");
            
            if (File.Exists(outputFile)) {
                File.Copy(outputFile, backupFile, true);
            }

            string output = JsonConvert.SerializeObject(ActiveUser, Formatting.Indented);
            File.WriteAllText(outputFile, output);
            Log.User.LogDebug("Saved ID Master achievements to {f}", outputFile);
        }
    }

    public static string RegisterSessionToken(UserData user) {
        Log.User.LogDebug("Registering random session token");
        return RegisterSessionToken(user, Utils.RandomString(32));
    }

    public static string RegisterSessionToken(UserData user, string token) {
        Log.User.LogDebug("Registering session token: {token}", token);
        activeTokens[token] = user;
        return token;
    }

    public static UserData GetUserDataFromToken(string str) {
        return GetUserDataFromToken(str, null);
    }

    public static UserData GetUserDataFromToken(string str, HttpContextBase ctx) {
        if (Program.Config.GetBool("LAHEE", "TrustedMode") && ctx != null && ctx.Request.Source.IpAddress == "127.0.0.1") {
            return ActiveUser;
        }

        if (Program.Config.GetBool("LAHEE", "AutoSessionOnSingleUser") && userData.Count == 1) {
            UserData u = userData.Values.First();
            Log.User.LogDebug("AutoSession enabled, user is {u}", u);
            return u;
        }

        return activeTokens.GetValueOrDefault(str, null);
    }

    internal static UserData[] GetAllUserData() {
        return userData.Values.ToArray();
    }
}