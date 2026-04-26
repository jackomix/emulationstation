using System.Collections;
using Microsoft.Extensions.Logging;

namespace Haruka.Common.Configuration;

public class IniParser : IniFile {
    private static readonly char[] SEPARATOR = new char[] { '=' };

    private readonly Hashtable keyPairs = new Hashtable();

    private struct SectionPair {
        public string Section;
        public string Key;
    }

    /// <summary>
    /// Opens the INI file at the given path and enumerates the values in the IniParser.
    /// </summary>
    /// <param name="iniPath">Full path to INI file.</param>
    public IniParser(string iniPath) : base(iniPath) {
        StreamReader iniFile = null;
        string currentRoot = null;

        iniPath = Path;

        if (File.Exists(iniPath)) {
            try {
                iniFile = new StreamReader(iniPath);

                string strLine = iniFile.ReadLine();

                while (strLine != null) {
                    strLine = strLine.Trim();

                    if (strLine != "") {
                        if (strLine.StartsWith('[') && strLine.EndsWith(']')) {
                            currentRoot = strLine.Substring(1, strLine.Length - 2);
                        } else {
                            string[] keyPair = strLine.Split(SEPARATOR, 2);

                            SectionPair sectionPair;
                            string value = null;

                            if (currentRoot == null) {
                                currentRoot = "ROOT";
                            }

                            sectionPair.Section = currentRoot;
                            sectionPair.Key = keyPair[0];

                            if (keyPair.Length > 1) {
                                value = keyPair[1];
                            }

                            keyPairs[sectionPair] = value;
                        }
                    }

                    strLine = iniFile.ReadLine();
                }
            } finally {
                iniFile?.Close();
            }
        }
    }

    public override string Read(string key, string section) {
        Log.Conf.LogDebug("Reading " + Path + ": " + (section != null ? "[" + section + "] " : "") + "" + key);
        SectionPair sectionPair;
        sectionPair.Section = section;
        sectionPair.Key = key;

        Log.Conf.LogDebug("Read Result: " + keyPairs[sectionPair]);

        return (string)keyPairs[sectionPair];
    }

    public override List<string> GetKeys(string section) {
        ArrayList tmpArray = new ArrayList();

        foreach (SectionPair pair in keyPairs.Keys) {
            if (pair.Section == section)
                tmpArray.Add(pair.Key);
        }

        return new List<string>((string[])tmpArray.ToArray(typeof(string)));
    }

    public override void Write(string key, string value, string section) {
        SectionPair sectionPair;
        sectionPair.Section = section;
        sectionPair.Key = key;

        if (keyPairs.ContainsKey(sectionPair)) {
            keyPairs.Remove(sectionPair);
        }

        keyPairs.Add(sectionPair, value);
        SaveSettings();
    }

    /// <summary>
    /// Save settings to new file.
    /// </summary>
    /// <param name="newFilePath">New file path.</param>
    public void SaveSettings(string newFilePath) {
        ArrayList sections = new ArrayList();
        string strToSave = "";

        foreach (SectionPair sectionPair in keyPairs.Keys) {
            if (!sections.Contains(sectionPair.Section)) {
                sections.Add(sectionPair.Section);
            }
        }

        foreach (string section in sections) {
            strToSave += "[" + section + "]\r\n";

            foreach (SectionPair sectionPair in keyPairs.Keys) {
                if (sectionPair.Section == section) {
                    string tmpValue = (string)keyPairs[sectionPair];

                    if (tmpValue != null) {
                        tmpValue = "=" + tmpValue;
                    }

                    strToSave += sectionPair.Key + tmpValue + "\r\n";
                }
            }

            strToSave += "\r\n";
        }

        StreamWriter sw = new StreamWriter(newFilePath);
        sw.Write(strToSave);
        sw.Close();
    }

    /// <summary>
    /// Save settings back to ini file.
    /// </summary>
    public void SaveSettings() {
        SaveSettings(Path);
    }
}