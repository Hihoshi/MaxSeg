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

template <class Key, class Value>
class HashTable
{
private:
    size_t table_size_;                             // 哈希表桶的数量
    std::optional<std::pair<Key, Value>> *buckets_; // 存储哈希表桶的数组

public:
    // 构造函数，初始化指定大小的哈希表
    HashTable(size_t table_size) :
    table_size_(table_size),
    buckets_(new std::optional<std::pair<Key, Value>>[table_size]())
    {
        if (table_size == 0)
            throw std::invalid_argument("Table size must be greater than zero");
    }

    // 禁用拷贝构造函数
    HashTable(const HashTable &) = delete;

    // 禁用拷贝赋值运算符
    HashTable &operator=(const HashTable &) = delete;

    // 移动构造函数（noexcept保证不会抛出异常）
    HashTable(HashTable &&other) noexcept
        : table_size_(other.table_size_), buckets_(other.buckets_)
    {
        other.table_size_ = 0;
        other.buckets_ = nullptr;
    }

    // 移动赋值运算符
    HashTable &operator=(HashTable &&other) noexcept
    {
        if (this != &other)
        {
            delete[] buckets_;
            table_size_ = other.table_size_;
            buckets_ = other.buckets_;
            other.table_size_ = 0;
            other.buckets_ = nullptr;
        }
        return *this;
    }

    // 计算键的哈希值（模运算确定桶位置）
    size_t hash(const Key &key) const
    {
        return std::hash<Key>{}(key) % table_size_;
    }

    // 检查键是否存在，返回存在位置或table_size_表示不存在
    size_t exists(const Key &key, const size_t pos = std::numeric_limits<size_t>::max()) const
    {
        size_t current_pos = (pos == std::numeric_limits<size_t>::max()) ? hash(key) : pos;
        return (buckets_[current_pos].has_value() && buckets_[current_pos]->first == key) ? current_pos : table_size_;
    }

    // 基础操作（不检查边界，调用前需先验证exists）
    Value get(const size_t pos) const { return buckets_[pos]->second; }

    void erase(const Key &key) { buckets_[hash(key)].reset(); }

    void insert(const std::pair<Key, Value> &pair)
    {
        buckets_[hash(pair.first)] = pair;
    }

    // 下标运算符重载
    std::optional<std::pair<Key, Value>> &operator[](const size_t pos)
    {
        return buckets_[pos];
    }

    const std::optional<std::pair<Key, Value>> &operator[](const size_t pos) const
    {
        return buckets_[pos];
    }

    size_t size() const { return table_size_; }

    ~HashTable() { delete[] buckets_; }
};

template <class Key, class Value>
class MultiHashTable
{
private:
    std::vector<HashTable<Key, Value>> tables_;           // 多层哈希表结构
    std::vector<std::pair<Key, Value>> overflow_entries_; // 溢出处理容器

    // 质数判断函数（6k±1优化法）
    bool is_prime(size_t n) const
    {
        if (n < 2)
            return false;
        if (n == 2 || n == 3)
            return true;
        if (n % 6 != 1 && n % 6 != 5)
            return false;
        size_t sqrt_n = static_cast<size_t>(std::sqrt(n));
        for (size_t i = 5; i <= sqrt_n; i += 6)
            if (n % i == 0 || n % (i + 2) == 0)
                return false;
        return true;
    }

public:
    // 多层哈希表构造函数（按指数衰减策略生成质数层）
    MultiHashTable(unsigned int layers = 3, size_t initial_size = 1e5, float exp_factor = 0.6)
    {
        if (initial_size < 2)
            throw std::invalid_argument("Initial size too small");
        if (exp_factor <= 0 || exp_factor >= 1)
            throw std::invalid_argument("Invalid factor");

        tables_.reserve(layers);
        for (unsigned int i = 0; i < layers; ++i)
        {
            size_t upper_bound = static_cast<size_t>(initial_size * pow(exp_factor, i));

            // 寻找不大于upper_bound的最大质数
            while (upper_bound >= 2 && !is_prime(upper_bound))
                --upper_bound;
            if (upper_bound < 2)
                throw std::invalid_argument("Prime not found");
            tables_.emplace_back(upper_bound);
        }
    }

    // 查找操作（按层查找 + 溢出遍历）
    std::optional<Value> get(const Key &key) const
    {
        const size_t hash_value = std::hash<Key>{}(key);
        for (const auto &table : tables_)
        {
            const size_t pos = table.exists(key, hash_value % table.size());
            if (pos != table.size())
                return table.get(pos);
        }
        for (const auto &entry : overflow_entries_)
            if (entry.first == key)
                return entry.second;
        return std::nullopt;
    }

    // 删除操作（按层删除 + 溢出删除）
    void erase(const Key &key)
    {
        const size_t hash_value = std::hash<Key>{}(key);
        for (auto &table : tables_)
        {
            const size_t pos = table.exists(key, hash_value % table.size());
            if (pos != table.size())
            {
                table[pos].reset();
                return;
            }
        }
        overflow_entries_.erase(
            std::remove_if(
                overflow_entries_.begin(),
                overflow_entries_.end(),
                [&](const auto &entry) { return entry.first == key; }
            ),
            overflow_entries_.end()
        );
    }

    // 插入操作（逐层尝试 + 溢出兜底）
    void insert(const std::pair<Key, Value> &pair)
    {
        const size_t hash_value = std::hash<Key>{}(pair.first);
        for (auto &table : tables_)
        {
            const size_t pos = hash_value % table.size();
            if (!table[pos])
            {
                table[pos] = pair;
                return;
            }
            else if (table[pos]->first == pair.first)
            {
                table[pos]->second = pair.second;
                return;
            }
        }
        auto it = std::find_if(
            overflow_entries_.begin(),
            overflow_entries_.end(),
            [&](const auto &entry)
            { return entry.first == pair.first; });
        if (it != overflow_entries_.end())
            it->second = pair.second;
        else
            overflow_entries_.push_back(pair);
    }

    // 统计信息（总容量 + 溢出数量）
    std::pair<size_t, size_t> size() const
    {
        size_t total = 0;
        for (const auto &table : tables_)
            total += table.size();
        return {total, overflow_entries_.size()};
    }

    // 打印负载状态信息
    void info() const
    {
        std::cout << std::fixed << std::setprecision(2);
        for (size_t i = 0; i < tables_.size(); ++i)
        {
            const auto &table = tables_[i];
            size_t used = std::count_if(
                &table[0],
                &table[table.size()],
                [](const auto &bucket)
                { return bucket.has_value(); });
            std::cout
                << "Layer " << i << ": size=" << table.size()
                << ", used=" << used << " ("
                << (used * 100.0 / table.size()) << "%)\n";
        }
        std::cout << "Overflow entries: " << overflow_entries_.size() << "\n\n";
    }
};
