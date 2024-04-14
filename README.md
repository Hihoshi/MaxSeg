# MaxSeg
A full C++ based forward maiximum matching module to split Chinese sentence.
Use multihashlish to achieve O(1) matching within a vocabulary list of 26k lines.
KMP algorithm are used to match a single word during the forward pass.

完全使用C++实现的最大匹配中文分词模块。
使用多阶哈希表实现O(1)的查询复杂度，在26万词的词表中查询，
前向匹配单个词使用KMP算法

## test demo
run ```main.cpp``` to test
