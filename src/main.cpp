// #include "PreSplit.h"
// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <string>
// #include <chrono>
// #include <stdexcept>
// #include <windows.h>

// namespace
// {
//     constexpr const char *DATA_PATH = "data/dict.txt";
//     constexpr const char *TEST_PATH = "data/demo.txt";
//     constexpr size_t INITIAL_LAYERS = 10;
//     constexpr size_t MAX_CAPACITY = 1e5;
// }

// void load_data(MultiHashTable<::std::string, ::std::string> &table)
// {
//     ::std::ifstream file(DATA_PATH);
//     if (!file.is_open())
//     {
//         throw ::std::runtime_error("Failed to open dictionary file");
//     }

//     ::std::string line;
//     while (::std::getline(file, line))
//     {
//         if (line.empty())
//             continue;

//         const size_t separator_pos = line.find("=>");
//         if (separator_pos == ::std::string::npos)
//             continue;

//         ::std::string word = line.substr(0, separator_pos);
//         ::std::string explanation = line.substr(separator_pos + 2);
//         table.insert(::std::move(::std::make_pair(::std::move(word), ::std::move(explanation))));
//     }
// }

// ::std::vector<::std::string> load_test()
// {
//     ::std::ifstream file(TEST_PATH);
//     if (!file.is_open())
//     {
//         throw ::std::runtime_error("Failed to open test file");
//     }

//     ::std::vector<::std::string> test_data;
//     ::std::string line;
//     while (::std::getline(file, line))
//     {
//         if (!line.empty())
//         {
//             test_data.push_back(::std::move(line));
//         }
//     }
//     return test_data;
// }


// int main() {
//     SetConsoleOutputCP(CP_UTF8);
//     try {
//         MultiHashTable<::std::string, ::std::string> table(INITIAL_LAYERS, MAX_CAPACITY);
//         load_data(table);
    
//         ::std::vector<::std::string> test_sentences = load_test();
//         ::std::vector<::std::vector<::std::string>> results;
//         results.reserve(test_sentences.size());
    
//         const auto start_time = ::std::chrono::high_resolution_clock::now();
    
//         for (const auto &sentence : test_sentences)
//         {
//             results.push_back(MaxiumSplit(table, sentence));
//         }
    
//         const auto end_time = ::std::chrono::high_resolution_clock::now();
//         const auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end_time - start_time);
    
//         // 输出分词结果
//         for (const auto &segmentation : results)
//         {
//             for (const auto &word : segmentation)
//             {
//                 ::std::cout << word << " ";
//             }
//             ::std::cout << "\n";
//         }
    
//         // 输出性能统计
//         table.info();
//         ::std::cout << "Total time: " << duration.count() << " μs\n";
//     }
//     catch (const ::std::exception& e) {
//         ::std::cerr << "Error: " << e.what() << ::std::endl;
//         return 1;
//     }
//     return 0;
// }



#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>    
#include "MultiHashTable.h"

using namespace std;
using namespace std::chrono;


vector<string> generate_keys(size_t count, size_t key_len) {
    vector<string> keys;
    keys.reserve(count);
    static const char char_pool[] = 
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static const size_t pool_size = sizeof(char_pool) - 1; // 排除结尾的'\0'
    
    if (key_len == 0 || count == 0) {
        return keys;
    }
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<size_t> dist(0, pool_size - 1);
    unique_ptr<char[]> shared_buffer(new char[count * key_len]);
    unordered_set<string> seen;
    seen.reserve(count); // 预分配空间以减少哈希表重建
    const size_t max_attempts = count * 2;
    for (size_t i = 0; i < count; ++i) {
        char* buffer = shared_buffer.get() + i * key_len;
        string s;
        size_t attempts = 0;
        do {
            if (++attempts > max_attempts) {
                throw runtime_error("Exceeded maximum attempts to generate unique key.");
            }
            // 生成随机字符串
            for (size_t j = 0; j < key_len; ++j) {
                buffer[j] = char_pool[dist(rng)];
            }
            s.assign(buffer, key_len);
        } while (seen.count(s)); // 检查是否已存在
        seen.insert(s);
        keys.emplace_back(s);
    }
    return keys;
}


void rigorous_performance_test(size_t op_count, size_t key_len, size_t layers, size_t initial_size) {
    // 预生成所有测试数据
    cout << "Generating test data..." << endl;
    auto all_keys = generate_keys(op_count * 2, key_len); // 双倍数量用于不同测试
    
    const auto& insert_keys = all_keys;                   // 前op_count个用于插入
    const auto non_existing_keys = vector<string>(        // 后op_count个确保不存在
        all_keys.begin() + op_count, all_keys.end()
    );
    
    vector<int> values(op_count);
    iota(values.begin(), values.end(), 0);                // 生成0到op_count-1的连续值

    // 初始化测试对象（强制产生溢出）
    MultiHashTable<string, int> mht(layers, initial_size);

    // 阶段1: 纯插入性能测试
    {
        cout << "-- Insert Test --" << endl;
        auto start = high_resolution_clock::now();
        
        for (size_t i = 0; i < op_count; ++i) {
            mht.insert({insert_keys[i], values[i]});
        }
        
        auto duration = duration_cast<microseconds>(
            high_resolution_clock::now() - start
        );
        cout
            << "Insert " << op_count << " elements: "
            << duration.count() << " us (" 
            << static_cast<double>(duration.count()) / static_cast<double>(op_count) << " us/op)\n";
    }

    // 阶段2: 查询测试（50%存在，50%不存在）
    {
        cout << "\n-- Query Test --" << endl;
        vector<string> query_keys;
        query_keys.reserve(op_count * 2);
        
        // 混合存在和不存在键
        for (size_t i = 0; i < op_count; ++i) {
            query_keys.push_back(insert_keys[i]);         // 存在的键
            query_keys.push_back(non_existing_keys[i]);   // 不存在的键
        }
        shuffle(query_keys.begin(), query_keys.end(), mt19937{random_device{}()});

        size_t found_count = 0;
        auto start = high_resolution_clock::now();
        
        for (const auto& key : query_keys) {
            if (mht.get(key).has_value()) {
                ++found_count;
            }
        }
        
        auto duration = duration_cast<microseconds>(
            high_resolution_clock::now() - start
        );
        cout
            << "Query " << query_keys.size() << " elements ("
            << found_count << " hits): "
            << duration.count() << " us ("
            << static_cast<double>(duration.count()) / static_cast<double>(query_keys.size()) << " us/op)\n";
    }

    // 阶段3: 更新测试（仅更新存在的键）
    {
        cout << "\n-- Update Test --" << endl;
        vector<int> new_values(op_count);
        generate(new_values.begin(), new_values.end(), 
               [n=0]() mutable { return n++ * 2; });  // 生成新值

        auto start = high_resolution_clock::now();
        for (size_t i = 0; i < op_count; ++i) {
            mht.insert({insert_keys[i], new_values[i]});
        }
        
        auto duration = duration_cast<microseconds>(
            high_resolution_clock::now() - start
        );
        cout
            << "Update " << op_count << " elements: "
            << duration.count() << " us ("
            << static_cast<double>(duration.count()) / static_cast<double>(op_count) << " us/op)\n";
    }

    // 阶段4: 删除测试（删除50%元素）
    {
        cout << "\n-- Delete Test --" << endl;
        vector<string> delete_keys;
        delete_keys.reserve(op_count);
        
        for (size_t i = 0; i < op_count; i += 2) {
            delete_keys.push_back(insert_keys[i]);
        }
        shuffle(delete_keys.begin(), delete_keys.end(), mt19937{random_device{}()});

        auto start = high_resolution_clock::now();
        
        for (const auto& key : delete_keys) {
            mht.erase(key);
        }
        
        auto duration = duration_cast<microseconds>(
            high_resolution_clock::now() - start
        );
        cout
            << "Delete " << delete_keys.size() << " elements: "
            << duration.count() << " us ("
            << static_cast<double>(duration.count()) / static_cast<double>(delete_keys.size()) << " us/op)\n";
    }

    // 最终验证
    cout << "\n-- Final Verification --" << endl;
    size_t error_count = 0;
    
    // 验证存在的键
    for (size_t i = 1; i < op_count; i += 2) {  // 只检查未删除的键
        auto val = mht.get(insert_keys[i]);
        if ((!val || *val )!= i*2) {  // 应该更新为i*2
            ++error_count;
        }
    }
    
    // 验证删除的键
    for (size_t i = 0; i < op_count; i += 2) {
        if (mht.get(insert_keys[i]).has_value()) {
            ++error_count;
        }
    }
    
    // 验证不存在的键
    for (const auto& key : non_existing_keys) {
        if (mht.get(key).has_value()) {
            ++error_count;
        }
    }

    cout << "Verification errors: " << error_count << endl;

    mht.info();

    // 测试清空操作耗时
    auto start = high_resolution_clock::now();
    mht.clear();
    auto duration = duration_cast<microseconds>(
        high_resolution_clock::now() - start
    );
    cout << "Clear operation took " << duration.count() << " us." << endl;

}

int main() {
    try {
        const size_t test_size = 1e5;       // 测试规模
        const size_t key_length = 16;       // 键长度
        const size_t layers = 4;            // 层级数
        const size_t initial_size = 1e5;    // 初始大小

        cout << "===== Starting Strict Performance Test =====" << endl;
        cout << fixed << setprecision(2);
        cout << "Operations: " << test_size << "\nKey length: " << key_length << " chars\n" << endl;
        rigorous_performance_test(test_size, key_length, layers, initial_size);
        cout << "===== Test Completed =====" << endl;
    } catch (const exception& e) {
        cerr << "Test Failed: " << e.what() << endl;
        return 1;
    }
    return 0;
}
