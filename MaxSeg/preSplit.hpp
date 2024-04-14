#include <string>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <vector>
#include <queue>
#include <windows.h>
#include "multiHashList.hpp"

// change charsets
wstring UTF8ToUnicode(const string &s){
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    wchar_t *buffer = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buffer, len);
    wstring result = buffer;
    delete[] buffer;
    return result;
}
string UnicodeToUTF8(const wstring &s){
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, NULL, 0, NULL, NULL);
    char *buffer = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, buffer, len, NULL, NULL);
    string result = buffer;
    delete [] buffer;
    return result;
}

// KMP string matching
int* getNext(wstring p){
    int* next = new int[(int)p.length()];
    next[0] = -1;           //while the first char not match, i++,j++
    int j = 0, k = -1;
    while (j < (int)p.length() - 1){
        if (k == -1 || p[j] == p[k]){
            j ++, k ++;
            next[j] = k;
        }
        else k = next[k];
    }
    return next;
}
int kmp(wstring T,wstring p){
    int i = 0, j = 0;
    int* next = getNext(T);
    while (i < (int)T.length() && j < (int)p.length()){
        if (j == -1 || T[i] == p[j]) i ++, j ++;
        else j = next[j];
    }
    if (j == (int)p.length()) return i - j;
    delete [] next;
    return -1;
}

// maxium split
vector<string> maxiumSplit(multiHashList<string> &table, string sen){
    wstring sentence = UTF8ToUnicode(sen);
    vector<string> candidates;
    int border = 0;
    for (int i = 0; i < (int)sentence.length(); i ++){
        wstring maxstr = L"", temp; // 最长的可匹配串
        int pos = i - 1;   // 最长可匹配串的末尾
        int flag = -1;    // 第一个可匹配串的末尾
        int max = 0;     // 最大的串长度
        int count = 0;  // 可以查到的串的计数
        int miss = 0;  // 匹配未命中次数
        for (int j = 1; i + j <= (int)sentence.length(); j ++){
            if (miss >= 4) break;  // 如果连续四次都匹配不上，认为之后没有可能匹配上更长的词
            temp = sentence.substr(i, j);
            if (table.exist(UnicodeToUTF8(temp))){
                count += 1;
                if (count == 1) flag = i + j - 1;
                if (j >= max){  // 保存最大串
                    maxstr = temp;
                    max = j;
                    pos = i + j - 1;
                    miss = 0;
                }
            }
            else miss ++;
        }
        if (maxstr != L"" && pos > border){
            candidates.push_back(UnicodeToUTF8(maxstr));
            border = pos;
        }
        if (flag != -1 && count >= 2) i = flag;  // 跳到第一个词的末尾开始
    }
    if (candidates.empty()) candidates.push_back(sen);  // 如果这个词不可再分，返回它本身
    return candidates;
}

// find all the pathes
vector<vector<string>> fullPath(vector<string> pre, string sen){
    vector<vector<string>> pathes;
    pathes.push_back(vector<string>());
    vector<int> borders(8, 0), pBorders(8, -1);  // previous border
    wstring sentence = UTF8ToUnicode(sen);
    queue<wstring> s;
    for (int i = 0; i < pre.size(); i ++) s.push(UTF8ToUnicode(pre[i]));
    while (!s.empty()){
        int pathesSize = pathes.size();
        for (int i = 0; i < pathesSize; i ++){
            int pos;  // pos是匹配词首的位置
            pos = kmp(sentence.substr(pBorders[i] + 1, (int)sentence.length()), s.front()) + pBorders[i] + 1;
            if (pos > borders[i]){  // 匹配词在边界右边
                pathes[i].push_back(UnicodeToUTF8(sentence.substr(borders[i], pos - borders[i])));  // 把匹配词前的加入
                pathes[i].push_back(UnicodeToUTF8(s.front()));  // 把匹配词加入
                pBorders[i] = borders[i];
                borders[i] = pos + (int)s.front().length();
            }
            else if (pos == borders[i]){
                pathes[i].push_back(UnicodeToUTF8(s.front()));
                pBorders[i] = borders[i];
                borders[i] = pos + (int)s.front().length();
            }
            else { // new
                vector<string> newPath;
                if (pathes.size() == borders.size()){
                    borders.push_back(0);
                    pBorders.push_back(-1);
                }
                for (int j = 0; j < pathes[i].size() - 1; j ++){
                    newPath.push_back(pathes[i][j]);
                    borders[pathes.size()] += (int)UTF8ToUnicode(pathes[i][j]).length();
                }
                if (pos = borders[pathes.size()]){
                    newPath.push_back(UnicodeToUTF8(sentence.substr(borders[pathes.size()] , pos - borders[pathes.size()])));
                }
                else {
                    newPath.push_back(UnicodeToUTF8(sentence.substr(borders[pathes.size()] , pos - borders[pathes.size()])));
                    newPath.push_back(UnicodeToUTF8(s.front()));
                }
                pBorders[i] = borders[i];
                pBorders[pathes.size()] = borders[pathes.size()];
                borders[pathes.size()] = pos + (int)s.front().length();
                pathes.push_back(newPath);
            }
        }
        s.pop();
    }
    for (int i = 0; i < pathes.size(); i ++)
        if (borders[i] < (int)sentence.length())
            pathes[i].push_back(UnicodeToUTF8(sentence.substr(borders[i], (int)sentence.length() - borders[i])));
    return pathes;
}
