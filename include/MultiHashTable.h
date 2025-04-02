#include <optional>
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <algorithm>
#include <limits>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <random>
#include <stdexcept>
#include <memory>
#include <map>
#include <tuple>
#include <cassert>


template <typename Key, typename Value>
class HashTable {
private:
    size_t table_size_;                             
    ::std::unique_ptr<::std::optional<::std::pair<Key, Value>>[]> buckets_;

public:
    HashTable(size_t table_size) :
    table_size_(table_size),
    buckets_(::std::make_unique<::std::optional<::std::pair<Key, Value>>[]>(table_size)) {
        if (table_size == 0)
            throw ::std::invalid_argument("Table size must be greater than zero");
    }

    // 删除拷贝构造函数和赋值运算符，以防止浅拷贝
    HashTable(const HashTable&) = delete;
    HashTable &operator=(const HashTable&) = delete;

    // 移动构造函数和赋值运算符，以实现深拷贝
    HashTable(HashTable &&other) noexcept
    : table_size_(other.table_size_), buckets_(std::move(other.buckets_)) {
        other.table_size_ = 0;
    }

    HashTable &operator=(HashTable &&other) noexcept {
        if (this != &other)
        {
            table_size_ = other.table_size_;
            buckets_ = std::move(other.buckets_);
            other.table_size_ = 0;
        }
        return *this;
    }

    ::std::optional<::std::pair<Key, Value>> &operator[](const size_t &pos) {
        assert(pos < table_size_);
        return buckets_[pos];
    }

    const ::std::optional<::std::pair<Key, Value>> &operator[](const size_t &pos) const {
        assert(pos < table_size_);
        return buckets_[pos];
    }

    size_t hash(const Key &key) const {
        return ::std::hash<Key>{}(key) % table_size_;
    }

    // 检查键是否存在，如果存在返回键在表中的位置，否则返回size_t的最大值（表示不存在）
    const ::std::optional<size_t> exists(
        const Key &key,
        const ::std::optional<size_t> pos = ::std::nullopt
    ) const {
        // 如果pos有值，则直接使用pos作为当前位置，否则计算哈希值取余获得位置
        size_t current_pos = pos.value_or(hash(key));
        // 检查当前位置是否有值，并且键是否匹配
        if (buckets_[current_pos].has_value() && (buckets_[current_pos].value()).first == key)
            return ::std::make_optional<size_t>(current_pos);
        else
            return ::std::nullopt;
    }

    // 获取键对应的值，如果不存在则返回空的optional对象
    ::std::optional<Value> get(const Key &key) const {
        ::std::optional<size_t> pos = exists(key);
        if (pos.has_value())
            return buckets_[pos]->second;
        else
            return ::std::nullopt;
    }

    // 获取指定位置的值，如果该位置不存在则返回空的optional对象
    ::std::optional<Value> get(const size_t &pos) const {
        assert(pos < table_size_);
        if (buckets_[pos].has_value())
            return buckets_[pos]->second;
        else
            return ::std::nullopt;
    }


    // 擦除指定位置的键值对，如果该位置不存在则不进行任何操作
    void erase(const Key &key, ::std::optional<size_t> pos = std::nullopt) {
        size_t current_pos = pos.value_or(hash(key));
        buckets_[current_pos].reset();
    }

    // 插入键值对，如果键已经存在则更新其对应的值
    void insert(const ::std::pair<Key, Value> &pair, ::std::optional<size_t> pos = std::nullopt) {
        size_t current_pos = pos.value_or(hash(pair.first));
        buckets_[current_pos] = pair;
    }

    const size_t size() const { return table_size_; }
};


template <typename Key, typename Value>
class MultiHashTable {

private:
    ::std::vector<HashTable<Key, Value>> tables_;           
    ::std::map<Key, Value> overflow_entries_;

    bool is_prime(size_t n) const
    {
        if (n < 2) return false;
        if (n == 2 || n == 3) return true;
        if (n % 6 != 1 && n % 6 != 5) return false;
        size_t sqrt_n = static_cast<size_t>(::std::sqrt(n));
        for (size_t i = 5; i <= sqrt_n; i += 6)
            if (n % i == 0 || n % (i + 2) == 0)
                return false;
        return true;
    }

public:
    MultiHashTable(unsigned int layers = 10, size_t initial_size = 1e5) {
        if (initial_size < 2)
            throw ::std::invalid_argument("Initial size too small");

        tables_.reserve(layers);
        size_t upper_bound = initial_size;
        for (unsigned int i = 0; i < layers; ++i) {
            while (upper_bound >= 2 && !is_prime(upper_bound))
                --upper_bound;
            if (upper_bound < 2)
                throw ::std::invalid_argument("Size too small for finding proper prime");
            tables_.emplace_back(upper_bound);
            --upper_bound;
        }
    }


    ::std::optional<Value> get(const Key &key) const {
        const size_t hash_value = ::std::hash<Key>{}(key);
        for (const auto &table : tables_) {
            const ::std::optional<size_t> pos = table.exists(key, hash_value % table.size());
            if (pos.has_value())
                return table.get(pos.value());
        }
        auto it = overflow_entries_.find(key);
        if (it != overflow_entries_.end())
            return it->second;
        return ::std::nullopt;
    }


    void erase(const Key &key) {
        const size_t hash_value = ::std::hash<Key>{}(key);
        for (auto &table : tables_) {
            const ::std::optional<size_t> pos = table.exists(key, hash_value % table.size());
            if (pos.has_value()) {
                table[pos.value()].reset();
                return;
            }
        }
        overflow_entries_.erase(key);
    }


    void insert(const ::std::pair<Key, Value> &pair) {
        const size_t hash_value = ::std::hash<Key>{}(pair.first);
        for (auto &table : tables_) {
            const size_t pos = hash_value % table.size();
            if (!table[pos].has_value()) {
                table[pos] = pair;
                return;
            }
            else if (table[pos]->first == pair.first) {
                table[pos]->second = pair.second;
                return;
            }
        }
        overflow_entries_[pair.first] = pair.second;
        return;
    }


    const ::std::tuple<size_t, size_t> size() const {
        size_t total = 0;
        for (const auto &table : tables_)
            total += table.size();
        return {total, overflow_entries_.size()};
    }


    void info() const {
        ::std::cout << "MultiHashTable info:\n";
        ::std::cout << ::std::fixed << ::std::setprecision(2);
        size_t used_count = 0;
        for (size_t i = 0; i < tables_.size(); ++i)
        {
            const auto &table = tables_[i];
            size_t used = ::std::count_if(
                &table[0],
                &table[table.size() - 1],
                [](const auto &bucket)
                { return bucket.has_value(); }
            );
            used_count += used;
            ::std::cout
                << "Layer " << i << ": size=" << table.size()
                << ", used=" << used << " ("
                << (used * 100.0 / table.size()) << "%)\n";
        }
        ::std::cout
            << "Total size=" << std::get<0>(this->size()) 
            << "  Total used="<< used_count << "  Average load factor="
            << (used_count * 100.0 / std::get<0>(this->size())) << "%"  << "\n";
        ::std::cout << "Overflow entries=" << overflow_entries_.size() << "\n\n";
    }
};
