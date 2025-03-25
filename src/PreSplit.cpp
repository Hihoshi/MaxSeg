#include "MultiHashTable.hpp"
#include <windows.h>
#include <string>
#include <codecvt>
#include <locale>
#include <vector>
#include <optional>


std::wstring utf8_to_unicode(const std::string& utf8_str) {
    if (utf8_str.empty()) return L"";

    int count = MultiByteToWideChar(
        CP_UTF8, 0, 
        utf8_str.c_str(), 
        static_cast<int>(utf8_str.length()), 
        nullptr, 0
    );
    if (count == 0) return L"";

    std::wstring wstr;
    wstr.resize(count);
    MultiByteToWideChar(
        CP_UTF8, 0, 
        utf8_str.c_str(), 
        static_cast<int>(utf8_str.length()), 
        &wstr[0], count
    );
    return wstr;
}

std::string unicode_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    int count = WideCharToMultiByte(
        CP_UTF8, 0, 
        wstr.c_str(), 
        static_cast<int>(wstr.length()), 
        nullptr, 0, 
        nullptr, nullptr
    );
    if (count == 0) return "";

    std::string utf8_str;
    utf8_str.resize(count);
    WideCharToMultiByte(
        CP_UTF8, 0, 
        wstr.c_str(), 
        static_cast<int>(wstr.length()), 
        &utf8_str[0], count, 
        nullptr, nullptr
    );
    return utf8_str;
}

struct MatchInfo
{
    std::wstring longest_match = L"";   // 最长匹配子串
    int longest_end_pos = -1;     // 最长匹配结束位置
    int first_match_end_pos = -1; // 首个匹配结束位置
    int match_count = 0;          // 匹配总数
};

constexpr int MAX_CONSECUTIVE_MISSES = 4; // 最大允许连续未命中次数

MatchInfo find_max_match(
    const MultiHashTable<std::string, std::string> &table,
    const std::wstring &sentence,
    size_t start_pos
)
{
    MatchInfo result;
    int consecutive_misses = 0;
    size_t max_length = 0;

    for (size_t length = 1; start_pos + length <= sentence.size(); ++length)
    {
        const auto current_sub = sentence.substr(start_pos, length);
        const auto utf8_sub = unicode_to_utf8(current_sub);
        const auto is_match = table.get(utf8_sub).has_value();

        if (!is_match)
        {
            if (++consecutive_misses >= MAX_CONSECUTIVE_MISSES)
                break;
            else
                continue;
        }

        // 更新匹配信息
        consecutive_misses = 0;
        ++result.match_count;

        // 更新首个匹配位置
        if (result.match_count == 1)
        {
            result.first_match_end_pos = start_pos + length - 1;
        }

        // 更新最长匹配信息
        if (length > max_length)
        {
            max_length = length;
            result.longest_match = current_sub;
            result.longest_end_pos = start_pos + length - 1;
        }
    }
    return result;
}

// 最大匹配分词，把每一个匹配到的词都放入结果中
// 例如 “提高人民生活水平”
// 把所有可能的匹配词（能在table中查到的，都分离出来）
// 得到 “提高 高人 人民 民生 生活 水平”
std::vector<std::string> MaxiumSplit(
    const MultiHashTable<std::string, std::string> &table,
    const std::string &sentence)
{
    const auto wide_sentence = utf8_to_unicode(sentence);
    std::vector<std::string> candidates;
    int last_end_pos = -1;

    for (size_t i = 0; i < wide_sentence.size();)
    {
        const auto match_info = find_max_match(table, wide_sentence, i);

        // 添加有效匹配
        if (!match_info.longest_match.empty() && match_info.longest_end_pos > last_end_pos)
        {
            candidates.emplace_back(unicode_to_utf8(match_info.longest_match));
            last_end_pos = match_info.longest_end_pos;
        }

        // 确定下一个起始位置
        if (match_info.match_count >= 2)
            i = match_info.first_match_end_pos + 1; // 跳过已匹配部分
        else
            ++i; // 常规步进
    }

    if (candidates.empty())
        candidates.push_back(sentence);
    return candidates;
}
