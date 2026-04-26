using Haruka.Common.Configuration;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using NReco.Logging.File;

namespace Haruka.Common;

public static class Log {
    public static ILogger Main { get; private set; }
    public static ILogger Conf { get; private set; }
    public static Dictionary<string, ILogger> Loggers { get; private set; }

    private static ILoggerFactory factory;

    public static void Initialize() {
        Loggers = new Dictionary<string, ILogger>();
        
        IConfigurationSection loggingConfig = AppConfig.Primary.GetSection("Logging");

        factory = LoggerFactory.Create(builder => builder
            .AddConfiguration(loggingConfig)
            .AddSimpleConsole(options => { options.SingleLine = true; })
            .AddDebug()
            .AddFile(loggingConfig.GetSection("File"))
        );
        Main = GetOrCreate("Main");
        Conf = GetOrCreate("Conf");
        
        Main.LogInformation("Logging started.");
    }

    public static ILogger GetOrCreate(string key) {
        if (Loggers.TryGetValue(key, out ILogger value)) {
            return value;
        }

        value = factory.CreateLogger(key);
        Loggers[key] = value;

        return value;
    }
}