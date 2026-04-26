namespace Haruka.Common.Util;

public class Clipboard {

    private static string str = "";

    public static void Write(string value) {
        str = value;
    }

    public static string Read() {
        return str;
    }

}