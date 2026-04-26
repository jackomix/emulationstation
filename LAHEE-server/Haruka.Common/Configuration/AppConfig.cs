using Microsoft.Extensions.Configuration;

namespace Haruka.Common.Configuration;

public class AppConfig {
    public static AppConfig Primary { get; private set; }

    private readonly IConfigurationRoot config;

    public static void Initialize(string[] args = null) {
        Primary = new AppConfig("Haruka.Common.Settings", args);
    }

    public AppConfig(string prefix, string[] args = null) {
        var builder = new ConfigurationBuilder()
            .AddJsonFile(prefix + ".json", false)
            .AddJsonFile(prefix + ".debug.json", true, true)
            .AddJsonFile(prefix + ".local.json", true, true);

        if (args != null) {
            var switchMappings = new Dictionary<string, string> {
                { "--roms", "LAHEE:RomsDirectory" },
                { "--hub", "LAHEE:HubDirectory" },
                { "--data", "LAHEE:DataDirectory" },
                { "--badges", "LAHEE:BadgeDirectory" },
                { "--user", "LAHEE:UserDirectory" }
            };
            builder.AddCommandLine(args, switchMappings);
        }

        config = builder.Build();
    }

    public string Get(string section, string value) {
        return config.GetSection(section).GetSection(value)?.Value;
    }

    public string Get(string section, string subsection, string value) {
        return config.GetSection(section).GetSection(subsection).GetSection(value).Value;
    }

    public int GetInt(string section, string value) {
        return config.GetSection(section).GetValue<int>(value);
    }

    public int GetInt(string section, string subsection, string value) {
        return config.GetSection(section).GetSection(subsection).GetValue<int>(value);
    }

    public bool GetBool(string section, string value) {
        return config.GetSection(section).GetValue<bool>(value);
    }

    public bool GetBool(string section, string subsection, string value) {
        return config.GetSection(section).GetSection(subsection).GetValue<bool>(value);
    }

    public IConfigurationSection GetSection(string section) {
        return config.GetSection(section);
    }
}