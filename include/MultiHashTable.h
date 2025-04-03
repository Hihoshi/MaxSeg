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


template <typename Key, typename Value>
class HashTable {
private:
    size_t table_size_;                             
    ::std::unique_ptr<::std::optional<::std::pair<Key, Value>>[]> buckets_;


    // 私有的下标访问函数，用于内部操作，可以修改值
    ::std::optional<::std::pair<Key, Value>>& operator[](const size_t &pos) {
        if (pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        return buckets_[pos];
    }

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


    // 移动构造函数和移动赋值运算符
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


    // 访问元素，但不可以修改元素的值，如果索引超出范围则抛出异常
    const ::std::optional<::std::pair<Key, Value>>& at(const size_t &pos) const {
        if (pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        return buckets_[pos];
    }


    // 计算键的哈希值，并对表大小取余以确定存储位置
    size_t hash(const Key &key) const {
        return ::std::hash<Key>{}(key) % table_size_;
    }


    // 检查键是否存在，如果存在返回键在表中的位置，否则返回size_t的最大值（表示不存在）
    const ::std::optional<size_t> exists(
        const Key &key,
        const ::std::optional<size_t> pos = ::std::nullopt
    ) const {
        // 如果pos有值，则直接使用pos作为当前位置，否则计算哈希值取余获得位置
        const size_t current_pos = pos.value_or(hash(key));
        if (current_pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        // 检查当前位置是否有值，并且键是否匹配
        if (buckets_[current_pos].has_value() && (buckets_[current_pos].value()).first == key)
            return current_pos;
        else
            return ::std::nullopt;
    }


    // 获取键对应的值，如果不存在则返回空
    ::std::optional<Value> get(const Key &key) const {
        const ::std::optional<size_t> pos = exists(key);
        if (pos.has_value())
            return buckets_[pos]->second;
        else
            return ::std::nullopt;
    }


    // 获取指定位置的值，如果该位置不存在则返回空
    ::std::optional<Value> get(const size_t &pos) const {
        if (pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        if (buckets_[pos].has_value())
            return buckets_[pos]->second;
        else
            return ::std::nullopt;
    }


    // 擦除指定位置的键值对，如果该位置不存在则不进行任何操作
    void erase(const Key &key, ::std::optional<size_t> pos = std::nullopt) {
        const size_t current_pos = pos.value_or(hash(key));
        if (pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        buckets_[current_pos].reset();
    }

    // 插入键值对，如果键已经存在则更新其对应的值
    void insert(const ::std::pair<Key, Value> &pair, ::std::optional<size_t> pos = std::nullopt) {
        const size_t current_pos = pos.value_or(hash(pair.first));
        if (pos >= table_size_)
            throw ::std::invalid_argument("Index out of range");
        buckets_[current_pos] = pair;
    }

    void clear(void) {
        for (size_t i = 0; i < table_size_; ++i) {
            buckets_[i].reset();
        }
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
            const size_t pos = hash_value % table.size();
            const ::std::optional<size_t> existence_pos = table.exists(key, pos);
            if (existence_pos.has_value())
                return table.get(existence_pos.value());
        }
        
        auto it = overflow_entries_.find(key);
        if (it != overflow_entries_.end())
            return it->second;
        return ::std::nullopt;
    }


    void erase(const Key &key) {
        const size_t hash_value = ::std::hash<Key>{}(key);
        for (auto &table : tables_) {
            const size_t pos = hash_value % table.size();
            const ::std::optional<size_t> existence_pos = table.exists(key, pos);
            if (existence_pos.has_value()) {
                table.erase(key, existence_pos.value());
                return;
            }
        }
        overflow_entries_.erase(key);
    }


    void insert(const ::std::pair<Key, Value> &pair) {
        const size_t hash_value = ::std::hash<Key>{}(pair.first);
        for (auto &table : tables_) {
            const size_t pos = hash_value % table.size();
            if (!table.at(pos).has_value() || table.at(pos)->first == pair.first) {
                table.insert(pair, pos);
                return;
            }
        }
        overflow_entries_[pair.first] = pair.second;
    }

    void clear(void) {
        for (auto &table : tables_) {
            table.clear();
        }
        overflow_entries_.clear();
    }


    const ::std::tuple<size_t, size_t> size() const {
        size_t total = 0;
        for (const auto &table : tables_)
            total += table.size();
        return {total, overflow_entries_.size()};
    }


    void info() const {
        size_t total_used = 0;
        size_t total_size = 0;
        std::cout << "MultiHashTable Info:\n";
        for (size_t i = 0; i < tables_.size(); ++i) {
            const auto& table = tables_[i];
            size_t used = 0;
            for (size_t j = 0; j < table.size(); ++j) {
                if (table.at(j).has_value()) 
                    ++used;
            }
            total_used += used;
            total_size += table.size();
            std::cout
                << "Layer " << i << ": Size=" << table.size()
                << ", Used=" << used << " ("
                << (used * 100.0 / table.size()) << "%)\n";
        }
        std::cout
            << "Total Capacity: " << total_size
            << "\nTotal Used: " << total_used
            << " (" << (total_used * 100.0 / total_size) << "%)\n"
            << "Overflow Entries: " << overflow_entries_.size() << "\n\n";
    }
};
