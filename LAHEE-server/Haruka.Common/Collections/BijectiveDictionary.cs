namespace Haruka.Common.Collections {
    public class BijectiveDictionary<TKey, TValue> {
        private readonly EqualityComparer<TKey> keyComparer;
        private readonly Dictionary<TKey, ISet<TValue>> forwardLookup;
        private readonly EqualityComparer<TValue> valueComparer;
        private readonly Dictionary<TValue, ISet<TKey>> reverseLookup;

        public BijectiveDictionary()
            : this(EqualityComparer<TKey>.Default, EqualityComparer<TValue>.Default) {
        }

        public BijectiveDictionary(EqualityComparer<TKey> keyComparer, EqualityComparer<TValue> valueComparer) : this(0, keyComparer, valueComparer) {
        }

        public BijectiveDictionary(int capacity, EqualityComparer<TKey> keyComparer, EqualityComparer<TValue> valueComparer) {
            this.keyComparer = keyComparer;
            forwardLookup = new Dictionary<TKey, ISet<TValue>>(capacity, keyComparer);
            this.valueComparer = valueComparer;
            reverseLookup = new Dictionary<TValue, ISet<TKey>>(capacity, valueComparer);
        }

        public void Add(TKey key, TValue value) {
            AddForward(key, value);
            AddReverse(key, value);
        }

        public void AddForward(TKey key, TValue value) {
            if (!forwardLookup.TryGetValue(key, out ISet<TValue> values)) {
                values = new HashSet<TValue>(valueComparer);
                forwardLookup.Add(key, values);
            }

            values.Add(value);
        }

        public void AddReverse(TKey key, TValue value) {
            if (!reverseLookup.TryGetValue(value, out ISet<TKey> keys)) {
                keys = new HashSet<TKey>(keyComparer);
                reverseLookup.Add(value, keys);
            }

            keys.Add(key);
        }

        public bool TryGetReverse(TValue value, out ISet<TKey> keys) {
            return reverseLookup.TryGetValue(value, out keys);
        }

        public ISet<TKey> GetReverse(TValue value) {
            TryGetReverse(value, out ISet<TKey> keys);
            return keys;
        }

        public bool ContainsForward(TKey key) {
            return forwardLookup.ContainsKey(key);
        }

        public bool TryGetForward(TKey key, out ISet<TValue> values) {
            return forwardLookup.TryGetValue(key, out values);
        }

        public ISet<TValue> GetForward(TKey key) {
            TryGetForward(key, out ISet<TValue> values);
            return values;
        }

        public bool ContainsReverse(TValue value) {
            return reverseLookup.ContainsKey(value);
        }

        public void Clear() {
            forwardLookup.Clear();
            reverseLookup.Clear();
        }
    }
}