using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using System.Text;
using static Haruka.Common.WindowHandleInfo;

namespace Haruka.Common;

public static class NativeMethods {

    public const int SW_SHOWMINNOACTIVE = 7;
    public const int SW_RESTORE = 9;
    public const int SW_HIDE = 0;
    public const int SW_SHOW = 5;

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "SendMessage")]
    public static extern IntPtr SendMessageSB(IntPtr hWnd, uint msg, IntPtr wParam, [Out] StringBuilder lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString,
        int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextLength(IntPtr hWnd);

    // honestly I have no clue what is expected for me to write here. LPCTSTR is not a thing, LPTSTR and LPSTR only give blank outputs.
    // ¯\_(ツ)_/¯
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "3")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "2")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "1")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "0")]
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ThrowOnUnmappableChar = true)]
    public static extern bool WritePrivateProfileString(string section, string key, string value, string filePath);

    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "5")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "3")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "2")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "1")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "0")]
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ThrowOnUnmappableChar = true)]
    public static extern int GetPrivateProfileString(string section, string key, string @default, StringBuilder retVal, int size, string filePath);

    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "3")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "2")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "1")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "0")]
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ThrowOnUnmappableChar = true)]
    public static extern int GetPrivateProfileSectionNames(byte[] lpszReturnBuffer, int nSize, string lpFileName);

    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "3")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "2")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "1")]
    [SuppressMessage("Microsoft.Globalization", "CA2101:SpecifyMarshalingForPInvokeStringArguments", MessageId = "0")]
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ThrowOnUnmappableChar = true)]
    public static extern int GetPrivateProfileSection(string lpAppName, byte[] lpszReturnBuffer, int nSize, string lpFileName);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool EnumChildWindows(IntPtr window, EnumWindowProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr intPtr, int nCmdShow);

    [DllImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool AllocConsole();

    internal const uint STD_OUTPUT_HANDLE = 0xFFFFFFF5;
    [DllImport("kernel32.dll")]
    public static extern IntPtr GetStdHandle(uint nStdHandle);
    [DllImport("kernel32.dll")]
    public static extern int SetStdHandle(uint nStdHandle, IntPtr handle);

    [DllImport("kernel32.dll")]
    public static extern IntPtr GetConsoleWindow();

    [DllImport("user32.dll")]
    public static extern int MessageBox(IntPtr hWnd, string text, string caption, int options);

}

public class WindowHandleInfo {
    public delegate bool EnumWindowProc(IntPtr hwnd, IntPtr lParam);

    private IntPtr mainHandle;

    public WindowHandleInfo(IntPtr handle) {
        this.mainHandle = handle;
    }

    public List<IntPtr> GetAllChildHandles() {
        List<IntPtr> childHandles = new List<IntPtr>();

        GCHandle gcChildhandlesList = GCHandle.Alloc(childHandles);
        IntPtr pointerChildHandlesList = GCHandle.ToIntPtr(gcChildhandlesList);

        try {
            NativeMethods.EnumChildWindows(mainHandle, EnumWindow, pointerChildHandlesList);
        } finally {
            gcChildhandlesList.Free();
        }

        return childHandles;
    }

    private bool EnumWindow(IntPtr hWnd, IntPtr lParam) {
        GCHandle gcChildhandlesList = GCHandle.FromIntPtr(lParam);

        if (gcChildhandlesList.Target == null) {
            return false;
        }

        List<IntPtr> childHandles = gcChildhandlesList.Target as List<IntPtr>;
        childHandles.Add(hWnd);

        return true;
    }
}