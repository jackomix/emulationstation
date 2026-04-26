namespace Haruka.Common.Collections {
    public static class StreamExtensions {

        public static void Write(this Stream stream, byte[] arr) {
            stream.Write(arr, 0, arr.Length);
        }

    }
}
