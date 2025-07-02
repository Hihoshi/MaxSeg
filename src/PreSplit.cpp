#include "MultiHashTable.h"
#include <windows.h>
#include <string>
#include <codecvt>
#include <locale>
#include <vector>
#include <optional>


::std::wstring utf8_to_unicode(const ::std::string& utf8_str) {
    if (utf8_str.empty()) return L"";

    int count = MultiByteToWideChar(
        CP_UTF8, 0, 
        utf8_str.c_str(), 
        static_cast<int>(utf8_str.length()), 
        nullptr, 0
    );
    if (count == 0) return L"";

    ::std::wstring wstr;
    wstr.resize(count);
    MultiByteToWideChar(
        CP_UTF8, 0, 
        utf8_str.c_str(), 
        static_cast<int>(utf8_str.length()), 
        &wstr[0], count
    );
    return wstr;
}

::std::string unicode_to_utf8(const ::std::wstring& wstr) {
    if (wstr.empty()) return "";

    int count = WideCharToMultiByte(
        CP_UTF8, 0, 
        wstr.c_str(), 
        static_cast<int>(wstr.length()), 
        nullptr, 0, 
        nullptr, nullptr
    );
    if (count == 0) return "";

    ::std::string utf8_str;
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
    ::std::wstring longest_match = L"";   // 最长匹配子串
    int longest_end_pos = -1;     // 最长匹配结束位置
    int first_match_end_pos = -1; // 首个匹配结束位置
    int match_count = 0;          // 匹配总数
};

constexpr int MAX_CONSECUTIVE_MISSES = 4; // 最大允许连续未命中次数


MatchInfo find_max_match(
    const MultiHashTable<::std::string, ::std::string>& table,
    const ::std::wstring& sentence,
    size_t start_pos
) {
    MatchInfo result;
    int consecutive_misses = 0;
    size_t max_length = 0;
    const size_t max_pos = sentence.size();
    for (size_t end_pos = start_pos + 1; end_pos <= max_pos; ++end_pos) {
        const size_t length = end_pos - start_pos;
        const ::std::wstring current_substr = sentence.substr(start_pos, length);
        const ::std::string utf8_str = unicode_to_utf8(current_substr);
        if (table.get(utf8_str)) {
            ++result.match_count;
            if (result.first_match_end_pos == -1) {
                result.first_match_end_pos = static_cast<int>(end_pos - 1);
            }
            if (length > max_length) {
                max_length = length;
                result.longest_match = current_substr;
                result.longest_end_pos = static_cast<int>(end_pos - 1);
            }
            consecutive_misses = 0;
        } else {
            if (++consecutive_misses >= MAX_CONSECUTIVE_MISSES) {
                break;
            }
        }
    }
    return result;
}

// 完整的分词函数
::std::vector<::std::string> MaxiumSplit(
    const MultiHashTable<::std::string, ::std::string>& table,
    const ::std::string& sentence
) {
    const ::std::wstring w_sentence = utf8_to_unicode(sentence);
    ::std::vector<::std::string> result;
    size_t start_pos = 0;
    while (start_pos < w_sentence.size()) {
        MatchInfo match = find_max_match(table, w_sentence, start_pos);
        
        if (match.longest_end_pos != -1) {
            const size_t length = match.longest_end_pos - start_pos + 1;
            result.push_back(unicode_to_utf8(
                w_sentence.substr(start_pos, length)));
            start_pos = match.longest_end_pos + 1;
        } else {
            result.push_back(unicode_to_utf8(
                w_sentence.substr(start_pos, 1)));
            ++start_pos;
        }
    }
    return result;
}