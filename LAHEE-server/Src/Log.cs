using Haruka.Common.Configuration;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using NReco.Logging.File;

namespace LAHEE;

static class Log {
    public static ILogger Main { get; private set; }
    public static ILogger Network { get; private set; }
    public static ILogger Data { get; private set; }
    public static ILogger User { get; private set; }
    public static ILogger RCheevos { get; private set; }
    public static ILogger Websocket { get; private set; }

    private static ILoggerFactory factory;

    public static void Initialize(bool machineMode = false) {
        IConfigurationSection loggingConfig = AppConfig.Primary.GetSection("Logging");

        factory = LoggerFactory.Create(builder => {
            if (machineMode) {
                builder.SetMinimumLevel(LogLevel.None);
            } else {
                builder.AddConfiguration(loggingConfig);
                builder.AddSimpleConsole(options => {
                    options.SingleLine = true;
                    options.TimestampFormat = "[HH:mm:ss.fff] ";
                });
                builder.AddDebug();
                builder.AddFile(loggingConfig.GetSection("File"));
            }
        });

        Main = factory.CreateLogger("Main");
        Network = factory.CreateLogger("Net ");
        Data = factory.CreateLogger("Data");
        User = factory.CreateLogger("User");
        RCheevos = factory.CreateLogger("Rche");
        Websocket = factory.CreateLogger("Webs");

        Main.LogInformation("Local Achievements Home Enhanced Edition " + Program.NAME);
    }
}