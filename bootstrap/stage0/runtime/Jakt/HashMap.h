/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Jakt/HashTable.h>
#include <Jakt/Optional.h>
#include <initializer_list>

namespace Jakt {

template<typename K, typename V, typename KeyTraits, typename ValueTraits, bool IsOrdered>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
    };

    struct EntryTraits {
        static unsigned hash(Entry const& entry) { return KeyTraits::hash(entry.key); }
        static bool equals(Entry const& a, Entry const& b) { return KeyTraits::equals(a.key, b.key); }
    };

public:
    using KeyType = K;
    using ValueType = V;

    HashMap() = default;

    HashMap(std::initializer_list<Entry> list)
    {
        MUST(ensure_capacity(list.size()));
        for (auto& item : list)
            MUST(set(item.key, item.value));
    }

    [[nodiscard]] bool is_empty() const
    {
        return m_table.is_empty();
    }
    [[nodiscard]] size_t size() const { return m_table.size(); }
    [[nodiscard]] size_t capacity() const { return m_table.capacity(); }
    void clear() { m_table.clear(); }
    void clear_with_capacity() { m_table.clear_with_capacity(); }

    ErrorOr<HashSetResult> set(const K& key, const V& value) { return m_table.try_set({ key, value }); }
    ErrorOr<HashSetResult> set(const K& key, V&& value) { return m_table.try_set({ key, move(value) }); }
    ErrorOr<HashSetResult> set(K&& key, V&& value) { return m_table.try_set({ move(key), move(value) }); }

    bool remove(const K& key)
    {
        auto it = find(key);
        if (it != end()) {
            m_table.remove(it);
            return true;
        }
        return false;
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) bool remove(Key const& key)
    {
        auto it = find(key);
        if (it != end()) {
            m_table.remove(it);
            return true;
        }
        return false;
    }

    template<typename TUnaryPredicate>
    bool remove_all_matching(TUnaryPredicate predicate)
    {
        return m_table.template remove_all_matching([&](auto& entry) {
            return predicate(entry.key, entry.value);
        });
    }

    using HashTableType = HashTable<Entry, EntryTraits, IsOrdered>;
    using IteratorType = typename HashTableType::Iterator;
    using ConstIteratorType = typename HashTableType::ConstIterator;

    [[nodiscard]] IteratorType begin() { return m_table.begin(); }
    [[nodiscard]] IteratorType end() { return m_table.end(); }
    [[nodiscard]] IteratorType find(const K& key)
    {
        return m_table.find(KeyTraits::hash(key), [&](auto& entry) { return KeyTraits::equals(key, entry.key); });
    }
    template<typename TUnaryPredicate>
    [[nodiscard]] IteratorType find(unsigned hash, TUnaryPredicate predicate)
    {
        return m_table.find(hash, predicate);
    }

    [[nodiscard]] ConstIteratorType begin() const { return m_table.begin(); }
    [[nodiscard]] ConstIteratorType end() const { return m_table.end(); }
    [[nodiscard]] ConstIteratorType find(const K& key) const
    {
        return m_table.find(KeyTraits::hash(key), [&](auto& entry) { return KeyTraits::equals(key, entry.key); });
    }
    template<typename TUnaryPredicate>
    [[nodiscard]] ConstIteratorType find(unsigned hash, TUnaryPredicate predicate) const
    {
        return m_table.find(hash, predicate);
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) [[nodiscard]] IteratorType find(Key const& key)
    {
        return m_table.find(Traits<Key>::hash(key), [&](auto& entry) { return Traits<K>::equals(key, entry.key); });
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) [[nodiscard]] ConstIteratorType find(Key const& key) const
    {
        return m_table.find(Traits<Key>::hash(key), [&](auto& entry) { return Traits<K>::equals(key, entry.key); });
    }

    ErrorOr<void> ensure_capacity(size_t capacity) { return m_table.try_ensure_capacity(capacity); }

    Optional<typename ValueTraits::ConstPeekType> get(const K& key) const requires(!IsPointer<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    Optional<typename ValueTraits::ConstPeekType> get(const K& key) const requires(IsPointer<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    Optional<typename ValueTraits::PeekType> get(const K& key) requires(!IsConst<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) Optional<typename ValueTraits::PeekType> get(Key const& key)
    const requires(!IsPointer<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) Optional<typename ValueTraits::ConstPeekType> get(Key const& key)
    const requires(IsPointer<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) Optional<typename ValueTraits::PeekType> get(Key const& key)
    requires(!IsConst<typename ValueTraits::PeekType>)
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    [[nodiscard]] bool contains(const K& key) const
    {
        return find(key) != end();
    }

    template<HashCompatible<K> Key>
    requires(IsSame<KeyTraits, Traits<K>>) [[nodiscard]] bool contains(Key const& value)
    {
        return find(value) != end();
    }

    void remove(IteratorType it)
    {
        m_table.remove(it);
    }

    V& ensure(const K& key)
    {
        auto it = find(key);
        if (it != end())
            return it->value;
        auto result = set(key, V());
        VERIFY(result == HashSetResult::InsertedNewEntry);
        return find(key)->value;
    }

    template<typename Callback>
    V& ensure(K const& key, Callback initialization_callback)
    {
        auto it = find(key);
        if (it != end())
            return it->value;
        auto result = set(key, initialization_callback());
        VERIFY(result == HashSetResult::InsertedNewEntry);
        return find(key)->value;
    }

    [[nodiscard]] u32 hash() const
    {
        u32 hash = 0;
        for (auto& it : *this) {
            auto entry_hash = pair_int_hash(it.key.hash(), it.value.hash());
            hash = pair_int_hash(hash, entry_hash);
        }
        return hash;
    }

private:
    HashTableType m_table;
};

}
