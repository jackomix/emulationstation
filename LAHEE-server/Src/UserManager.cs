using LAHEE.Data;
using LAHEE.Util;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using WatsonWebserver.Core;

namespace LAHEE;

class UserManager {
    public static string UserDataDirectory { get; set; }
    public static UserData ActiveUser { get; set; }

    private static Dictionary<string, UserData> userData;
    private static Dictionary<string, UserData> activeTokens;
    private static readonly Lock SAVE_LOCK = new Lock();

    internal static void Initialize() {
        activeTokens = new Dictionary<string, UserData>();

        // PERSISTENCE: Read the path from the master pointer
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        string statePath = Path.Combine(hubDir, "active_profile.json");
        string loadDir = Program.Config.Get("LAHEE", "UserDirectory"); // Default

        if (File.Exists(statePath)) {
            try {
                var state = JsonConvert.DeserializeObject<Dictionary<string, string>>(File.ReadAllText(statePath));
                if (state != null && state.ContainsKey("path")) {
                    string p = state["path"];
                    if (Directory.Exists(p)) loadDir = p;
                }
            } catch (Exception ex) {
                Log.User.LogWarning("Failed to read active_profile.json: {e}", ex.Message);
            }
        }

        UserDataDirectory = loadDir;
        Load(loadDir);

        if (userData.Count == 0) {
            // DERIVE IDENTITY: Use the folder name as the username (e.g. /Profiles/John/Achievements -> John)
            string derivedName = Path.GetFileName(Path.GetDirectoryName(loadDir));
            if (string.IsNullOrEmpty(derivedName) || derivedName == "User" || derivedName == "Achievements") derivedName = "Player";
            
            Log.User.LogInformation("No users found in {d}. Creating derived profile: {u}", loadDir, derivedName);
            RegisterNewUser(derivedName);
            Save();
        }

        // Whoever is in this private folder IS the active user
        ActiveUser = userData.Values.FirstOrDefault();

        Log.User.LogInformation("Loaded profile: {u} from {d}", ActiveUser?.UserName, loadDir);
    }

    public static void SaveActiveUser() {
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        if (string.IsNullOrEmpty(hubDir)) return;

        string statePath = Path.Combine(hubDir, "active_profile.json");
        var state = new Dictionary<string, string> {
            { "path", UserDataDirectory }
        };
        File.WriteAllText(statePath, JsonConvert.SerializeObject(state, Formatting.Indented));
    }

    public static void Load(string dir) {
        userData = new Dictionary<string, UserData>(); // CRITICAL: Wipe brain before loading

        Log.User.LogInformation("Loading user profiles from {Dir}...", dir);

        if (!Directory.Exists(dir)) {
            Directory.CreateDirectory(dir);
            Log.User.LogTrace("Created directory");
        }

        foreach (string file in Directory.GetFiles(dir, "*.json")) {
            if (file.EndsWith(".bak")) continue;

            try {
                UserData data = JsonConvert.DeserializeObject<UserData>(File.ReadAllText(file));
                Migrate(data);
                userData[data.UserName.ToLower()] = data;
                Log.User.LogDebug("Loaded data for \"{User}\"", data.UserName);
            } catch (Exception ex) {
                Log.User.LogError("Failed to load data from " + file + ": " + ex);
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
            return null;
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
        Log.User.LogInformation("Registered new user: {User}", user.UserName);
        return user;
    }

    public static void Save() {
        Save(UserDataDirectory);
    }

    public static void Save(string dir) {
        if (ActiveUser == null) return;

        lock (SAVE_LOCK) {
            // ONLY save the active user to this profile folder to prevent cross-contamination
            UserData data = ActiveUser;
            if (data.AllowUse) {
                string outputFile = Path.Combine(dir, data.UserName + ".json");
                string backupFile = Path.Combine(dir, data.UserName + ".bak");
                if (File.Exists(outputFile)) {
                    File.Copy(outputFile, backupFile, true);
                }

                string output = JsonConvert.SerializeObject(data);
                if (String.IsNullOrWhiteSpace(output)) {
                    throw new IOException("Attempted to write empty/null user save data for " + data.UserName);
                }

                File.WriteAllText(outputFile, output);
                Log.User.LogDebug("Saved user data for {user}", data.UserName);
            }

            Log.User.LogInformation("User data was saved for {u}", data.UserName);
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