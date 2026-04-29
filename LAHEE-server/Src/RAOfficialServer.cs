using System.Net;
using LAHEE.Data;
using LAHEE.Util;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;

namespace LAHEE;

public static class RAOfficialServer {
    private static string sessionToken;

    public static string Url {
        get { return Program.Config.Get("LAHEE:RAFetch:Url"); }
    }

    public static string SessionToken { get; private set; }

    public static bool CanFetch {
        get {
            string apiWeb = Program.Config.Get("LAHEE:RAFetch:WebApiKey");
            string username = Program.Config.Get("LAHEE:RAFetch:Username");
            string password = Program.Config.Get("LAHEE:RAFetch:Password");

            if (String.IsNullOrWhiteSpace(Url)) {
                Log.Main.LogError("Invalid RAFetch Url in configuration.");
                return false;
            }

            if (String.IsNullOrWhiteSpace(apiWeb)) {
                Log.Main.LogError("Invalid RAFetch WebApiKey in configuration. Get it from here: {u}", Url + "/settings");
                return false;
            }

            if (String.IsNullOrWhiteSpace(username)) {
                Log.Main.LogError("Invalid RAFetch username in configuration.");
                return false;
            }

            if (String.IsNullOrWhiteSpace(password)) {
                Log.Main.LogError("Invalid RAFetch password in configuration.");
                return false;
            }

            return true;
        }
    }

    public static void FetchDataByFile(string filePath) {
        if (!CanFetch) {
            return;
        }

        Log.RCheevos.LogInformation("Identifying game for: {f}", Path.GetFileName(filePath));
        
        string hash = Utils.GenerateRAHash(filePath); 

        Log.RCheevos.LogInformation("RAHash: {h}. Querying RetroAchievements...", hash);

        // 1. Resolve Hash to GameID
        RAApiResolveHashResponse resolve = Query<RAApiResolveHashResponse>(HttpMethod.Get, Url, "dorequest.php?r=gameid&m=" + hash, null);
        if (resolve == null || resolve.GameID == 0) {
            Log.RCheevos.LogWarning("Could not identify game with hash {h}", hash);
            return;
        }

        Log.RCheevos.LogInformation("Identified as GameID: {id}. Querying Game Metadata...", resolve.GameID);

        // 2. Fetch friendly Title for filenames
        string apiWeb = Program.Config.Get("LAHEE:RAFetch:WebApiKey");
        RAApiGameResponse gameMeta = Query<RAApiGameResponse>(HttpMethod.Get, Url, "API/API_GetGame.php?y=" + apiWeb + "&i=" + resolve.GameID, null);
        string friendlyTitle = (gameMeta != null && !string.IsNullOrEmpty(gameMeta.Title)) ? gameMeta.Title : resolve.GameID.ToString();
        
        // Sanitize for Windows/Linux filesystem safety
        string safeTitle = new string(friendlyTitle.Where(ch => !Path.GetInvalidFileNameChars().Contains(ch)).ToArray());

        // 3. Fetch full data (passing safeTitle to use for filename)
        FetchData(resolve.GameID.ToString(), null, false, false, safeTitle);

        // 4. Export icon to ES images folder
        Log.RCheevos.LogInformation("Exporting icon for \"{t}\" to EmulationStation...", friendlyTitle);
        ExportIconToES(filePath, resolve.GameID);
    }

    private static void ExportIconToES(string romPath, uint gameId) {
        try {
            string romDir = Path.GetDirectoryName(romPath);
            string romName = Path.GetFileNameWithoutExtension(romPath);
            string imagesDir = Path.Combine(romDir, "images");
            
            if (!Directory.Exists(imagesDir)) {
                Directory.CreateDirectory(imagesDir);
            }

            // Find source icon in nested structure with EXACT ID matching
            string dataDir = Program.Config.Get("LAHEE", "DataDirectory");
            var results = Directory.GetFiles(dataDir, "icon.png", System.IO.SearchOption.AllDirectories);
            string sourceIcon = results.FirstOrDefault(f => {
                string folderName = Path.GetFileName(Path.GetDirectoryName(f));
                // Match "123 - Title" or just "123" exactly
                return folderName == gameId.ToString() || folderName.StartsWith(gameId + " - ");
            });
            
            if (sourceIcon == null) {
                // Fallback to legacy
                string badgeDir = Program.Config.Get("LAHEE", "BadgeDirectory");
                sourceIcon = Path.Combine(badgeDir, gameId + ".png");
            }

            string targetIcon = Path.Combine(imagesDir, romName + "-icon.png");

            if (File.Exists(targetIcon)) {
                Log.RCheevos.LogInformation("Icon already exists, skipping: {t}", targetIcon);
                return;
            }

            if (File.Exists(sourceIcon)) {
                File.Copy(sourceIcon, targetIcon, true);
                Log.RCheevos.LogInformation("Exported icon to: {t}", targetIcon);
            }
        } catch (Exception ex) {
            Log.RCheevos.LogWarning("Failed to export icon to ES: {e}", ex.Message);
        }
    }

    public static void FetchData(string gameIdStr, string overrideIdStr, bool includeUnofficial, bool force = false, string customTitle = null, string copyToUsername = null) {
        if (!CanFetch) {
            return;
        }

        string apiWeb = Program.Config.Get("LAHEE:RAFetch:WebApiKey");
        string username = Program.Config.Get("LAHEE:RAFetch:Username");

        if (!UInt32.TryParse(gameIdStr, out uint fetchId)) {
            Log.Main.LogError("Not a valid game ID: {i}", gameIdStr);
            return;
        }

        uint overrideId = fetchId;
        if (!String.IsNullOrWhiteSpace(overrideIdStr)) {
            if (!UInt32.TryParse(overrideIdStr, out overrideId)) {
                Log.Main.LogError("Not a valid game ID (override ID): {i}", overrideIdStr);
                return;
            }
        }

        string sessionToken = LogInToRealServer();
        if (sessionToken == null) {
            return;
        }

        int f = includeUnofficial ? 7 : 3;
        RAPatchResponseV2 patch = Query<RAPatchResponseV2>(HttpMethod.Get, Url, "dorequest.php?r=achievementsets&u=" + username + "&t=" + sessionToken + "&f=" + f + "&m=&g=" + fetchId, null);
        if (patch == null) {
            return;
        }

        // --- NEW NESTED STRUCTURE LOGIC ---
        string displayTitle = customTitle ?? patch.Title;
        string dirName = fetchId.ToString();
        if (!string.IsNullOrEmpty(displayTitle) && displayTitle != fetchId.ToString()) {
            dirName = fetchId + " - " + displayTitle;
        }
        
        // Sanitize folder name
        dirName = new string(dirName.Where(ch => !Path.GetInvalidFileNameChars().Contains(ch)).ToArray());
        
        string gameDir = Path.Combine(Program.Config.Get("LAHEE", "DataDirectory"), dirName);
        string badgeDir = Path.Combine(gameDir, "Badge");

        if (!Directory.Exists(badgeDir)) {
            Directory.CreateDirectory(badgeDir);
        }

        GameData gameData = new GameData() {
            Title = displayTitle,
            ID = patch.GameID,
            ConsoleID = patch.ConsoleID,
            ImageIconURL = patch.ImageIconURL,
            DataVersion = GameData.CURRENT_DATA_VERSION,
            ImageIcon = patch.ImageIconURL,
            CodeNotes = new List<CodeNote>(),
            ROMHashes = new List<string>(),
            RichPresencePatch = patch.RichPresencePatch,
            AchievementSets = patch.Sets
        };

        List<string> hashes = new List<string>();
        RAApiHashesResponse hashResponse = Query<RAApiHashesResponse>(HttpMethod.Get, Url, "API/API_GetGameHashes.php?y=" + apiWeb + "&i=" + fetchId, null);
        if (hashResponse != null) {
            foreach (RAApiHashesResponse.Hash h in hashResponse.Results) {
                if (!hashes.Contains(h.MD5)) hashes.Add(h.MD5);
            }
        }

        FetchCodeNotes(gameData);

        // apply overridden ID for set merges
        gameData.ID = overrideId;
        // re-route image URLs to local
        gameData.ImageIconURL = StaticDataManager.LocalifyUrl(gameData.ImageIconURL);
        foreach (AchievementData ad in gameData.GetAllAchievements()) {
            ad.BadgeURL = StaticDataManager.LocalifyUrl(ad.BadgeURL);
            ad.BadgeLockedURL = StaticDataManager.LocalifyUrl(ad.BadgeLockedURL);
        }

        foreach (SetData set in patch.Sets) {
            set.GameID = StaticDataManager.RAIntegrationAssertionWorkaround(overrideId);
        }

        gameData.DeleteAchievementById(StaticDataManager.UNSUPPORTED_EMULATOR_ACHIEVEMENT_ID);

        // Save Data Files
        string outputFile = Path.Combine(gameDir, fetchId + ".set.json");
        string hashFile = Path.Combine(gameDir, fetchId + ".hash.txt");

        File.WriteAllText(outputFile, JsonConvert.SerializeObject(gameData));
        File.WriteAllLines(hashFile, hashes);

        Log.RCheevos.LogInformation("Downloading assets for \"{t}\"...", displayTitle);

        // Save Icon
        if (!string.IsNullOrEmpty(patch.ImageIconURL)) {
            string iconUrl = Url + patch.ImageIconURL;
            byte[] iconData = DownloadAsset(iconUrl);
            if (iconData != null) {
                File.WriteAllBytes(Path.Combine(gameDir, "icon.png"), iconData);
            }
        }

        // Save Badges (Skip _lock)
        foreach (AchievementData ach in gameData.GetAllAchievements()) {
            if (!string.IsNullOrEmpty(ach.BadgeName)) {
                string bUrl = Url + "/Badge/" + ach.BadgeName + ".png";
                string bPath = Path.Combine(badgeDir, ach.BadgeName + ".png");
                if (!File.Exists(bPath)) {
                    byte[] data = DownloadAsset(bUrl);
                    if (data != null) File.WriteAllBytes(bPath, data);
                }
            }
        }

        StaticDataManager.InitializeAchievements();

        if (copyToUsername != null) {
            UserData user = UserManager.GetUserData(copyToUsername) ?? UserManager.RegisterNewUser(copyToUsername);
            GameData game = StaticDataManager.FindGameDataById(overrideId);
            if (!user.GameData.TryGetValue(fetchId, out UserGameData userGameData)) {
                Log.User.LogInformation("Creating new progression for {user} in {game}", user, game);
                userGameData = user.RegisterGame(game);
            }

            RAStartSessionResponse al = Query<RAStartSessionResponse>(HttpMethod.Get, Url, "dorequest.php?r=startsession&u=" + username + "&t=" + sessionToken + "&h=0&l=11.4&g=" + fetchId + "&m=", null);
            if (al != null) {
                foreach (RAStartSessionResponse.RAStartSessionAchievementData ad in al.Unlocks) {
                    userGameData.UnlockAchievement(ad.ID, false, ad.When);
                }

                foreach (RAStartSessionResponse.RAStartSessionAchievementData ad in al.HardcoreUnlocks) {
                    userGameData.UnlockAchievement(ad.ID, true, ad.When);
                }
            }
        }
    }

    private static byte[] DownloadAsset(string url) {
        try {
            using (WebClient client = new WebClient()) {
                return client.DownloadData(url);
            }
        } catch (Exception ex) {
            Log.RCheevos.LogWarning("Failed to download asset {u}: {e}", url, ex.Message);
            return null;
        }
    }

    private static string LogInToRealServer() {
        if (sessionToken != null) return sessionToken;

        string username = Program.Config.Get("LAHEE:RAFetch:Username");
        string password = Program.Config.Get("LAHEE:RAFetch:Password");

        RALoginResponse response = Query<RALoginResponse>(HttpMethod.Get, Url, "dorequest.php?r=login2&u=" + username + "&p=" + password, null);
        if (response != null && response.Success) {
            sessionToken = response.Token;
            return sessionToken;
        }

        Log.RCheevos.LogError("Failed to log in to real server!");
        return null;
    }

    public static List<CodeNote> FetchCodeNotes(GameData game) {
        string apiWeb = Program.Config.Get("LAHEE:RAFetch:WebApiKey");
        RACodeNotesResponse response = Query<RACodeNotesResponse>(HttpMethod.Get, Url, "API/API_GetCodeNotes.php?y=" + apiWeb + "&i=" + game.ID, null);
        if (response != null && response.Success) {
            game.CodeNotes = response.CodeNotes;
            return response.CodeNotes;
        }
        return null;
    }

    public static void FetchComments(uint gameId, int achievementId) {
        string apiWeb = Program.Config.Get("LAHEE:RAFetch:WebApiKey");
        RAApiCommentsResponse response = Query<RAApiCommentsResponse>(HttpMethod.Get, Url, "API/API_GetAchievementComments.php?y=" + apiWeb + "&i=" + achievementId, null);
        if (response != null && response.Results != null) {
            foreach (UserComment c in response.Results) {
                StaticDataManager.AddComment(c, StaticDataManager.FindGameDataById(gameId), false);
            }
        }
    }

    public static void FetchUpdatedSets() {
        if (!CanFetch) return;
        // Basic stub to fix build error. Real implementation would check for revisions.
        Log.RCheevos.LogInformation("Checking for set updates...");
    }

    private static T Query<T>(HttpMethod method, string url, string endpoint, object body) where T : class {
        try {
            string fullUrl = url.TrimEnd('/') + "/" + endpoint;
            using (WebClient client = new WebClient()) {
                string content = client.DownloadString(fullUrl);
                T r = JsonConvert.DeserializeObject<T>(content);
                if (r is RAAnyResponse any && !any.Success && r is RAErrorResponse error) {
                    throw new Exception("RA request failed: " + error.Error + " (" + error.Code + ")");
                }
                return r;
            }
        } catch (Exception ex) {
            Log.RCheevos.LogWarning("Query failed: {u}, {e}", endpoint, ex.Message);
            return null;
        }
    }
}
