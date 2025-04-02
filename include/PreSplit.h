#include "MultiHashTable.h"
#include <string>
#include <codecvt>
#include <locale>
#include <vector>
#include <optional>

// 编码转换工具函数
::std::wstring utf8_to_unicode(const ::std::string &utf8_str);
::std::string unicode_to_utf8(const ::std::wstring &wide_str);

struct MatchInfo
{
    ::std::wstring longest_match = L"";   // 最长匹配子串
    int longest_end_pos = -1;     // 最长匹配结束位置
    int first_match_end_pos = -1; // 首个匹配结束位置
    int match_count = 0;          // 匹配总数
};

constexpr int MAX_CONSECUTIVE_MISSES = 4; // 最大允许连续未命中次数

MatchInfo find_max_match(
    const MultiHashTable<::std::string, ::std::string> &table,
    const ::std::wstring &sentence,
    size_t start_pos
);


// 最大匹配分词，把每一个匹配到的词都放入结果中
// 例如 “提高人民生活水平”
// 把所有可能的匹配词（能在table中查到的，都分离出来）
// 得到 “提高 高人 人民 民生 生活 水平”
::std::vector<std::string> MaxiumSplit(
    const MultiHashTable<::std::string, ::std::string> &table,
    const ::std::string &sentence
);