#ifndef IO_H
#define IO_H

#include <algorithm>
// #include <cstdio>  // JIA remove printf
#include <cstring>
#include <string>
#include <vector>

#include "elementary_DT_MT.h"
#include "trie_DT_MT.h"

using namespace std;

vector<string> StrSplit(const string &str, const string &pattern);
int Count1(uint64_t num);
uint32_t GetIp(string str);

vector<Rule_DT_MT *> ReadRules(string rules_file, int rules_shuffle);
vector<Rule_DT_MT *> RulesPortPrefix(vector<Rule_DT_MT *> &rules,
                                     bool free_rules);

int FreeRules(vector<Rule_DT_MT *> &rules);
vector<Trace *> ReadTraces(string traces_file);
int FreeTraces(vector<Trace *> &traces);
vector<int> GenerateAns(vector<Rule_DT_MT *> &rules, vector<Trace *> &traces,
                        int force_test);
void PrintAns(string output_file, vector<int> &ans);
uint16_t htons(uint16_t hostshort);
uint32_t htonl(uint32_t hostlong);
uint16_t ntohs(uint16_t netshort);
uint32_t ntohl(uint32_t netlong);

#endif
