#include "PreSplit.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>


namespace
{
    constexpr const char *DATA_PATH = "data/dict.txt";
    constexpr const char *TEST_PATH = "data/demo.txt";
    constexpr size_t INITIAL_LAYERS = 8;
    constexpr size_t MAX_CAPACITY = 100000;
    constexpr float LOAD_FACTOR = 0.8f;
}

void load_data(MultiHashTable<std::string, std::string> &table)
{
    std::ifstream file(DATA_PATH);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open dictionary file");
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        const size_t separator_pos = line.find("=>");
        if (separator_pos == std::string::npos)
            continue;

        std::string word = line.substr(0, separator_pos);
        std::string explanation = line.substr(separator_pos + 2);
        table.insert(std::make_pair(std::move(word), std::move(explanation)));
    }
}

std::vector<std::string> load_test()
{
    std::ifstream file(TEST_PATH);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open test file");
    }

    std::vector<std::string> test_data;
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            test_data.push_back(std::move(line));
        }
    }
    return test_data;
}


int main() {
    try {
        system("chcp 65001>nul");
        MultiHashTable<std::string, std::string> table(INITIAL_LAYERS, MAX_CAPACITY, LOAD_FACTOR);
        load_data(table);
    
        std::vector<std::string> test_sentences = load_test();
        std::vector<std::vector<std::string>> results;
        results.reserve(test_sentences.size());
    
        const auto start_time = std::chrono::high_resolution_clock::now();
    
        for (const auto &sentence : test_sentences)
        {
            results.push_back(MaxiumSplit(table, sentence));
        }
    
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
        // 输出分词结果
        for (const auto &segmentation : results)
        {
            for (const auto &word : segmentation)
            {
                std::cout << word << " ";
            }
            std::cout << "\n";
        }
    
        // 输出性能统计
        table.info();
        std::cout << "Total time: " << duration.count() << " μs\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}