using Newtonsoft.Json;
using Newtonsoft.Json.Converters;

namespace LAHEE.Data;

public enum SetType {
    core,
    bonus,
    specialty,
    exclusive
}

public class SetData {
    public string Title;

    [JsonConverter(typeof(StringEnumConverter))]
    public SetType Type;

    [JsonProperty("AchievementSetId")]
    public int AchievementSetId;

    [JsonProperty("ID")]
    public int ID => AchievementSetId;

    // Standard properties
    [JsonProperty("GameID")]
    public uint GameID;

    [JsonProperty("GameId")]
    public uint GameId => GameID;

    private string _imageIconUrl = "";
    [JsonProperty("ImageIconURL")]
    public string ImageIconURL { get => _imageIconUrl; set => _imageIconUrl = value ?? ""; }

    [JsonProperty("ImageIconUrl")]
    public string ImageIconUrl => ImageIconURL;

    public List<AchievementData> Achievements;
    public List<LeaderboardData> Leaderboards;
    [JsonIgnore] public string FileSource;
}
