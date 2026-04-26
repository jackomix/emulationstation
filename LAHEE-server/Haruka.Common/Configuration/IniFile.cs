using System.Runtime.InteropServices;
using System.Text;
using Haruka.Common.Collections;
using Microsoft.Extensions.Logging;

namespace Haruka.Common.Configuration;

public class IniFile {
    public string Path { get; protected set; }

    public static IniFile New(string iniPath) {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
            Log.Conf.LogTrace("Using Windows accessor");
            return new IniFile(iniPath);
        } else {
            Log.Conf.LogTrace("Using custom parser");
            return new IniParser(iniPath);
        }
    }

    protected IniFile(string iniPath) {
        Path = new FileInfo(iniPath).FullName;
        Log.Conf.LogInformation("Preparing " + Path);
    }

    public virtual string Read(string key, string section) {
        Log.Conf.LogDebug("Reading " + GetFileName() + ": " + (section != null ? "[" + section + "] " : "") + "" + key);

        StringBuilder buf = new StringBuilder(65535);
        buf.Clear();
        NativeMethods.GetPrivateProfileString(section, key, String.Empty, buf, buf.MaxCapacity, Path);
        Log.Conf.LogDebug("Read Result: " + buf);

        return buf.ToString();
    }

    private string GetFileName() {
        return System.IO.Path.GetFileName(Path);
    }

    public virtual void Write(string key, string value, string section) {
        Log.Conf.LogInformation("Updating " + GetFileName() + ": " + (section != null ? "[" + section + "] " : "") + "" + key + " -> " + value);

        NativeMethods.WritePrivateProfileString(section, key, value, Path);
    }

    public void Write(string key, object value, string section) {
        Write(key, value?.ToString(), section);
    }

    public void DeleteKey(string key, string section) {
        Write(key, null, section);
    }

    public void DeleteSection(string section) {
        Write(null, null, section);
    }

    public bool KeyExists(string key, string section) {
        return (Read(key, section) ?? "").Length > 0;
    }

    public virtual List<string> GetSections() {
        Log.Conf.LogDebug("Reading " + GetFileName() + ": Querying sections", "Configuration");

        byte[] buf = new byte[65535];
        buf.Fill<byte>(0);
        NativeMethods.GetPrivateProfileSectionNames(buf, buf.Length, Path);
        string allSections = Encoding.ASCII.GetString(buf);
        string[] sectionNames = allSections.Split('\0');
        List<string> s = new List<string>();
        foreach (string sectionName in sectionNames) {
            if (sectionName != String.Empty) {
                s.Add(sectionName);
            }
        }

        return s;
    }

    public virtual List<string> GetKeys(string section) {
        Log.Conf.LogDebug("Reading " + GetFileName() + ": " + (section != null ? "[" + section + "] " : "") + "Querying keys");

        byte[] buf = new byte[65535];
        buf.Fill<byte>(0);
        NativeMethods.GetPrivateProfileSection(section, buf, buf.Length, Path);
        string[] tmp = Encoding.ASCII.GetString(buf).Trim('\0').Split('\0');

        List<string> result = new List<string>();

        foreach (string entry in tmp) {
            if (!entry.StartsWith('#') && !entry.StartsWith(';')) {
                int index = entry.IndexOf('=');
                if (index > -1) {
                    result.Add(entry.Substring(0, index));
                }
            }
        }

        return result;
    }

    public string ReadString(string key, string section, string def = null) {
        string s = Read(key, section);
        return String.IsNullOrEmpty(s) ? def : s;
    }

    public int ReadInt(string key, string section, int def = 0) {
        string s = Read(key, section);
        return Int32.TryParse(s, out int i) ? i : def;
    }

    public bool ReadBool(string key, string section, bool def = false) {
        string s = Read(key, section);
        return Boolean.TryParse(s, out bool b) ? b : def;
    }
}