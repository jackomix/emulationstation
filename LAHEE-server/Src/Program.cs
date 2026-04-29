using System.Reflection;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using Haruka.Common.Configuration;
using LAHEE.Data;
using LAHEE.Util;
using Microsoft.Extensions.Logging;
using Microsoft.VisualBasic.FileIO;

namespace LAHEE;

class Program {
    internal static readonly char[] INVALID_FILE_NAME_CHARS = Path.GetInvalidFileNameChars();

    public static readonly string NAME;

    public static AppConfig Config;
    public static List<string> Notifications = new List<string>();
    public static bool IsMachineMode = false;

    static Program() {
        string gitHash = Assembly.Load(typeof(Program).Assembly.FullName!)
            .GetCustomAttributes<AssemblyMetadataAttribute>()
            .FirstOrDefault(attr => attr.Key == "GitHash")?.Value;

        AssemblyName assemblyInfo = Assembly.GetExecutingAssembly().GetName();
        NAME = assemblyInfo.Name + "/" + assemblyInfo.Version + "-" + gitHash + " - Akechi Haruka";
    }

    private static void Main(string[] args) {
        Console.Title = NAME;

        string path = Environment.ProcessPath;
        if (path != null) {
            Environment.CurrentDirectory = Path.GetDirectoryName(path)!;
        }

        try {
            AppConfig.Initialize(args);
            Config = new AppConfig("LAHEE", args);

            string hubDir = Config.Get("LAHEE", "HubDirectory");
            string dataDir = Config.Get("LAHEE", "DataDirectory");

            if (!string.IsNullOrEmpty(hubDir)) {
                Environment.CurrentDirectory = hubDir;
            } else if (!string.IsNullOrEmpty(dataDir) && Path.IsPathRooted(dataDir)) {
                Environment.CurrentDirectory = dataDir;
            }

            string badgeDirectory = Config.Get("LAHEE", "BadgeDirectory") ?? "Badge";
            if (!Directory.Exists(badgeDirectory)) {
                Directory.CreateDirectory(badgeDirectory);
            }
        } catch (Exception ex) {
            Console.WriteLine("An error ocurred during loading the configuration:\n" + ex.Message);
            Console.ReadLine();
            return;
        }

        IsMachineMode = args.Contains("--machine");
        Log.Initialize(IsMachineMode);

        try {
            UserManager.Initialize();
            StaticDataManager.Initialize();
            Network.Initialize();
            LiveTicker.Initialize();
            CaptureManager.Initialize();
        } catch (Exception ex) {
            Log.Main.LogCritical("An internal error occurred:\n" + ex.Message);
            Console.ReadLine();
            return;
        }

        if (!IsMachineMode) {
            Log.Main.LogInformation("Initialization complete.");
            Console.WriteLine("Type \"stop\" to save and exit.\nType \"help\" for console commands.\nPoint your emulator to: " + Network.LocalUrl);
        }

        if (Config.GetBool("LAHEE", "RAFetch", "CheckSetRevisions")) {
            new Thread(RAOfficialServer.FetchUpdatedSets) {
                Name = "Check Set Revisions"
            }.Start();
        }

        Console.CancelKeyPress += Console_CancelKeyPress;

        // --- CLI COMMAND PARSER ---
        List<string> commandList = new List<string>();
        for (int i = 0; i < args.Length; i++) {
            if (args[i].StartsWith("--")) {
                if ((args[i] == "--hub" || args[i] == "--roms" || args[i] == "--data") && i + 1 < args.Length) i++;
                continue;
            }
            commandList.Add(args[i]);
        }

        if (commandList.Count > 0) {
            string[] cmdArgs = commandList.ToArray();
            try {
                ExecuteConsoleCommand(cmdArgs);
            } catch (Exception ex) {
                if (IsMachineMode) Console.WriteLine($"ERROR: {ex.Message}");
                else Log.Main.LogError("Error executing CLI command: {e}", ex);
            }
            
            string cmd = cmdArgs[0].ToLower();
            if (cmd == "fetch" || cmd == "scrape") {
                Console_CancelKeyPress(null, null);
                return;
            }
        }

        while (true) {
            string line = Console.ReadLine();
            if (line == null) break;
            if (String.IsNullOrWhiteSpace(line)) continue;

            try {
                ExecuteConsoleCommand(ParseConsoleCommand(line));
            } catch (Exception ex) {
                Log.Main.LogError("Error executing console command: {e}", ex);
            }
        }
    }

    private static void ExecuteConsoleCommand(string[] args) {
        switch (args[0].ToLower()) {
            case "help":
                Console.WriteLine("scrape <dir> - Bulk scrape achievements");
                Console.WriteLine("stop - Exit");
                break;
            case "exit":
            case "quit":
            case "stop":
                Console_CancelKeyPress(null, null);
                break;
            case "fetch":
                if (args.Length >= 2) RAOfficialServer.FetchData(args[1], null, false);
                break;
            case "scrape":
                if (args.Length >= 2) {
                    string scanDir = args[1];
                    string[] extensions = { ".nes", ".sfc", ".smc", ".gb", ".gbc", ".gba", ".gen", ".sms", ".gg", ".pce", ".vboy", ".wsc", ".iso", ".chd", ".pbp", ".md", ".bin" };
                    
                    if (IsMachineMode) Console.WriteLine($"STATUS:Scanning folder {scanDir}...");
                    
                    if (!Directory.Exists(scanDir)) {
                        if (IsMachineMode) Console.WriteLine("ERROR:Directory not found: " + scanDir);
                        break;
                    }

                    List<string> allRoms = new List<string>();
                    Queue<string> dirs = new Queue<string>();
                    dirs.Enqueue(scanDir);

                    while (dirs.Count > 0) {
                        string currentDir = dirs.Dequeue();
                        if (currentDir.ToLower().Contains("retroachievements")) continue;

                        try {
                            // Find ROMs
                            string[] files = Directory.GetFiles(currentDir);
                            foreach (string f in files) {
                                string ext = Path.GetExtension(f).ToLower();
                                if (extensions.Contains(ext)) {
                                    allRoms.Add(f);
                                }
                            }

                            // Find Subdirs
                            foreach (string d in Directory.GetDirectories(currentDir)) {
                                string name = Path.GetFileName(d);
                                if (name.StartsWith("$") || name == "System Volume Information" || name == "RECYCLE.BIN") continue;
                                dirs.Enqueue(d);
                            }
                        } catch (Exception ex) {
                            if (IsMachineMode) Console.WriteLine($"DEBUG:Skipped {currentDir} - {ex.Message}");
                        }
                    }

                    int total = allRoms.Count;
                    if (IsMachineMode) Console.WriteLine($"STATUS:Found {total} games. Starting downloads...");
                    
                    int current = 0;
                    foreach (var rom in allRoms) {
                        current++;
                        if (IsMachineMode) {
                            Console.WriteLine($"PROGRESS:{current}:{total}");
                            Console.WriteLine($"STATUS:Processing {Path.GetFileName(rom)}");
                        }
                        try {
                            RAOfficialServer.FetchDataByFile(rom);
                        } catch (Exception ex) {
                            if (IsMachineMode) Console.WriteLine($"ERROR:Failed {Path.GetFileName(rom)}: {ex.Message}");
                        }
                    }
                    if (IsMachineMode) Console.WriteLine("STATUS:Scrape complete.");
                }
                break;
            default:
                Log.Main.LogWarning("Unknown command: {arg}", args[0]);
                break;
        }
    }

    private static string[] ParseConsoleCommand(string line) {
        string[] args;
        using (MemoryStream ms = new MemoryStream(Encoding.ASCII.GetBytes(line))) {
            using (TextFieldParser tfp = new TextFieldParser(ms)) {
                tfp.Delimiters = new string[] { " " };
                tfp.HasFieldsEnclosedInQuotes = true;
                args = tfp.ReadFields();
            }
        }
        return args;
    }

    private static void Console_CancelKeyPress(object sender, ConsoleCancelEventArgs e) {
        Network.Stop();
        UserManager.Save();
        StaticDataManager.SaveAllCommentFiles();
        StaticDataManager.SaveGlobalData();
        Environment.Exit(0);
    }

    public static void AddNotification(string message) {
        Notifications.Add(message);
        LiveTicker.BroadcastNotification(message);
    }
}
