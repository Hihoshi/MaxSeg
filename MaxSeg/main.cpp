#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <windows.h>
#include "preSplit.hpp"
using namespace std;

// load hashtable
multiHashList<string>& loadDict(){
    multiHashList<string> *table = new multiHashList<string>(10, 1e5);
    fstream fp;
    fp.open("E:\\workspace\\C++\\source\\wordSegmentation\\dict.txt", ios::in);
    while (!fp.eof()){
        string line = "", word = "", explanation = "";
        getline(fp, line);
        if (line == "") continue;
        word = line.substr(0, line.find("=>"));
        explanation = line.substr(line.find("=>") + 2, (int)line.length());
        table->add(pair<string, string>(word, explanation));
    }
    fp.close();
    return *table;
}

// load test sentences
vector<string>& loadTest(){
    fstream fp;
    fp.open("E:\\workspace\\C++\\source\\wordSegmentation\\demo.txt", ios::in);
    vector<string> *testdata = new vector<string>;
    while (!fp.eof()){
        string line = "";
        getline(fp, line);
        if (line == "") continue;
        testdata->push_back(line);
    }
    fp.close();
    return *testdata;
}


int main(void){
    multiHashList<string> table = loadDict();
    vector<string> test = loadTest();
    vector<vector<vector<string>>> res;
    // performence estimation
    LARGE_INTEGER t1, t2, tc;
    QueryPerformanceFrequency(&tc);
    // count time
    QueryPerformanceCounter(&t1);
    for (int i = 0; i < test.size(); i ++){
        try {
            vector<string> splited = maxiumSplit(table, test[i]);
            vector<vector<string>> pathes = fullPath(splited, test[i]);
            res.push_back(pathes);
        } catch(std::out_of_range) {
            cout << "line " << i << " error" << endl;
        }
    }
    // end count
    QueryPerformanceCounter(&t2);
    double time = (double)(t2.QuadPart - t1.QuadPart) / (double)tc.QuadPart; 
    for (int i = 0; i < res.size(); i ++){
        for (int j = 0; j < res[i].size(); j ++){
            for (int k = 0; k < res[i][j].size(); k ++) cout << res[i][j][k] << " ";
            cout << endl;
        }
        cout << endl;
    }
    // 输出时间（单位：us）
    cout << "耗时" << time * 1e6 << "us" << endl;
    table.info();
    return 0;
}