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

        UserDataDirectory = Program.Config.Get("LAHEE", "UserDirectory");

        Load(UserDataDirectory);

        if (userData.Count == 0) {
            Log.User.LogInformation("No users found in {d}. Creating profile...", UserDataDirectory);
            RegisterNewUser("Player");
            Save();
        }

        ActiveUser = userData.Values.First();

        Log.User.LogInformation("Finished loading data: {users} User(s)", userData.Count);
    }

    private static void RestoreActiveUser() {
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        if (string.IsNullOrEmpty(hubDir)) return;

        string path = Path.Combine(hubDir, "active_user.txt");
        if (File.Exists(path)) {
            string name = File.ReadAllText(path).Trim();
            if (userData.ContainsKey(name.ToLower())) {
                ActiveUser = userData[name.ToLower()];
                Log.User.LogInformation("Restored active user: {u}", name);
            }
        }
    }

    public static void SaveActiveUser() {
        string hubDir = Program.Config.Get("LAHEE", "HubDirectory");
        if (string.IsNullOrEmpty(hubDir) || ActiveUser == null) return;

        string path = Path.Combine(hubDir, "active_user.txt");
        File.WriteAllText(path, ActiveUser.UserName);
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
        foreach (UserGameData ugd in data.GameData.Values) {
            if (ugd.FlaggedAchievements == null) {
                ugd.FlaggedAchievements = new List<int>();
            }
        }
    }

    public static UserData GetUserData(string username) {
        if (userData.ContainsKey(username)) {
            return userData[username];
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
        userData.Add(username, user);
        Log.User.LogInformation("Registered new user: {User}", user);
        return user;
    }

    public static void Save() {
        Save(UserDataDirectory);
    }

    public static void Save(string dir) {
        lock (SAVE_LOCK) {
            foreach (UserData data in userData.Values) {
                if (data.AllowUse) {
                    string outputFile = Path.Combine(dir, data.UserName + ".json");
                    string backupFile = Path.Combine(dir, data.UserName + ".bak");
                    if (File.Exists(outputFile)) {
                        File.Copy(outputFile, backupFile, true);
                    }

                    string output = JsonConvert.SerializeObject(data);
                    if (String.IsNullOrWhiteSpace(output)) {
                        throw new IOException("Attempted to write empty/null user save data for " + data);
                    }

                    File.WriteAllText(outputFile, output);
                    Log.User.LogDebug("Saved user data for {user}", data.UserName);
                } else {
                    Log.User.LogWarning("Not saving {User}, because data loading has failed!", data);
                }
            }

            Log.User.LogInformation("User data was saved");
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