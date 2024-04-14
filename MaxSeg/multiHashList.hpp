#include <iostream>
#include <algorithm>
#include <math.h>
using namespace std;


template<class T>
class HashList{
    public:
    int len;
    hash<string>* hasher;
    pair<string, string>* pairs;
    HashList(int Len, hash<string>* Hasher){
        len = Len;
        hasher = Hasher;
        pairs = new pair<string, string>[len];
    }
    pair<string, T>& operator[](const unsigned long long pos){
        if (pos < len && pos >= 0) return pairs[pos];
        else throw out_of_range("hash value out of range");
    }
    pair<string, T>& operator[](const string Key){
        unsigned long long pos = (*hasher)(Key) % len;
        if (pos >= len || pos < 0) throw out_of_range("hash value out of range");
        return pairs[pos];
    }
    ~HashList(){
        delete [] pairs;
        len = 0;
        (*hasher).~hash();
    }
};


template<class T>
class multiHashList{
    public:
    int len;
    int scale;
    int* tableLen;
    HashList<T>** tables;
    hash<string>* hasher;
    bool isPrime(unsigned long long n){
        if (n == 2|| n == 3) return 1;
        if (n % 6 != 1 && n % 6 != 5) return 0;
        for (unsigned long long i = 5; i <= floor(sqrt(n)); i += 6)
            if (n % i == 0 || n % (i + 2) == 0) return 0;
        return 1;
    }
    multiHashList(int Len = 10, int Scale = 1e6){
        len = Len;
        scale = Scale;
        tableLen = new int[len];
        hasher = new hash<string>;
        for (int i = 0; i < len; i ++){
            int flag = 0, j;
            // 从scale开始倒序搜寻质数
            for (j = scale; !isPrime(j); j --){
                // j已经小于零，停止搜寻
                if (j < 2) {
                    // 保存本层序号退出
                    flag = 1;
                    break;
                }
            }
            scale /= 1.3; // 经验系数，使表更小，节约空间
            tableLen[i] = j;
            // 如果本层搜寻已使j小于零，到此为止
            if (flag == 1){
                // 上一层还没有让j等于零
                len = i - 1;
                break;
            }
        }
        tables = new HashList<T> *[len];
        for (int i = 0; i < len; i ++) tables[i] = new HashList<T>(tableLen[i], hasher);
    }
    pair<string, T> &operator[](string Key){
        unsigned long long hashvalue = (*hasher)(Key);
        for (int i = 0; i < len; i ++){
            unsigned long long pos = hashvalue % tableLen[i];
            if ((*tables[i])[pos].first == Key) return (*tables[i])[pos];
        }
        throw out_of_range("key no found");
    }
    // 判断 key 的位置是否为空
    bool aval(string Key){
        unsigned long long hashvalue = (*hasher)(Key);
        for (int i = 0; i < len; i ++){
            unsigned long long pos = hashvalue % tableLen[i];
            if ((*tables[i])[pos].first == "") return 1;
        }
        return 0;
    }
    // 判断 key 的位置是否存在他自己
    bool exist(string Key){
        unsigned long long hashvalue = (*hasher)(Key);
        for (int i = 0; i < len; i ++){
            unsigned long long pos = hashvalue % tableLen[i];
            if ((*tables[i])[pos].first == Key && (*tables[i])[pos].first != "") return 1;
        }
        return 0;
    }
    bool add(pair<string, T> thePair){
        unsigned long long hashvalue = (*hasher)(thePair.first);
        for (int i = 0; i < len; i ++){
            unsigned long long pos = hashvalue % tableLen[i];
            if ((*tables[i])[pos].first == ""){
                (*tables[i])[pos].first = thePair.first;
                (*tables[i])[pos].second = thePair.second;
                return 1;
            }
        }
        return 0;
    }
    void info(){
        int totalLoadedItems = 0;
        int loadedItems = 0;
        int capacity = 0;
        cout << "table :" << endl;
        for (int i = 0; i < len; i ++){
            cout << "list[" << i << "] Length:" << tableLen[i];
            loadedItems = 0;
            for (int j = 0; j < tableLen[i]; j ++){
                if (tables[i]->pairs[j].first != ""){
                    totalLoadedItems ++;
                    loadedItems ++;
                }
            }
            capacity += tableLen[i];
            cout << " Percentage: " << float(loadedItems) / float(tableLen[i]) << endl;
        }
        cout << "loaded: " << totalLoadedItems << "/" << capacity << " Percentage: " << float(totalLoadedItems) / float(capacity) << endl;
        return;
    }
    ~multiHashList(){
        for (int i = 0; i < len; i ++) tables[i]->~HashList();
        (*hasher).~hash();
        delete [] tableLen;
        len = 0;
        scale = 0;
    }
};