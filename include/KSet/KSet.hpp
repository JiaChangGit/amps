#ifndef __KSET_HPP__
#define __KSET_HPP__

#include <cmath>
#include <iomanip>  // 用於設置小數點位數

#include "KSet_data_structure.hpp"
using namespace std;

#define PTR_SIZE 4

// precompte and decide the usedbits in each Set
struct anaSet {
  size_t tablesize;
  size_t non_empty_seg = 0;

  int* seg;  // rules in segment
};

// big segment用priority效能會變差, rmd不會
struct BigSegment {
  int uf = -1;      // used exact field
  int bf = -1;      // -1: not use, 0: list, 1: hash
  int ub = 0;       // hash table used bits
  int ht_size = 0;  // hash table size

  vector<Rule> list;  // smaller than threshold2
  vector<Rule>* ht;   // need to hash
};

struct SegmentNode {
  int nrules = 0;
  int flag = -1;  // -1 = NULL, 0 = Small, 1 = Big

  vector<Rule> classifier;

  // four hash table or list, the number is the search order
  BigSegment b[4];

  int r_flag = -1;  // 0: rmd, 1: rmdp
  int r_max_pri = 0;
  int rp_max_pri[3] = {0};
  vector<Rule> rmd;      // remainder
  vector<Rule> rmdp[3];  // partition with protocol, tcp, udp, other
};

static inline int part_oder(int n) {
  int r = -1;

  switch (n) {
    case 0:
    case 1:
      r = n;
      break;
    case 2:
      r = 3;
      break;
    case 3:
      r = 2;
      break;
  }

  return r;
}

static inline int hashSet0(uint32_t orig, int k) {
  uint32_t v = orig;
  int uk = 32 - k;

  switch (k) {
    case 4:
      v = (v * 0x88888889) >> uk;
      break;
    case 5:
    case 6:
    case 7:
    case 8:
      v = (v * 0x80808081) >> uk;
      break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
      v = (v * 0x80008001) >> uk;
      break;
    default:
      cout << ("error k\n");
      v = 0;
  }

  return v;
}

static inline int exactHash(uint32_t v, int ub) {
  uint32_t res = v;

  int r_ub = 32 - ub;

  switch (ub) {
    case 1:
    case 2:
      res = (res * 0xAAAAAAAB) >> r_ub;
      break;
    case 3:
    case 4:
      res = (res * 0x88888889) >> r_ub;
      break;
    case 5:
    case 6:
    case 7:
    case 8:
      res = (res * 0x80808081) >> r_ub;
      break;
    case 9:
    case 10:
      res = (res * 0x80008001) >> r_ub;
      break;
    default:
      cout << "error usedbits: " << ub << endl;
      res = 0;
  }

  return res;
}

static inline bool cmpp(Rule const& a, Rule const& b) {
  return a.priority > b.priority;
}

class KSet {
 public:
  KSet(int num1, vector<Rule>& classifier, int usedbits);
  ~KSet();
  KSet(const KSet& other);                 // 複製建構子
  KSet& operator=(const KSet& other);      // 複製指派
  KSet(KSet&& other) noexcept;             // 移動建構子
  KSet& operator=(KSet&& other) noexcept;  // 移動指派

  virtual void ConstructClassifier(const vector<Rule>& rules);
  virtual int ClassifyAPacket(const Packet& packet);
  virtual void DeleteRule(const Rule& delete_rule);
  virtual void InsertRule(const Rule& insert_rule);

  void prints() {
    total_tablesize_memory_in_KB = (double)(tablesize * (PTR_SIZE)) / 1024;
    total_linear_memory_in_KB = (double)(Total_Rules_in_Linear_Node * 28) /
                                1024;  // rule: 24 bytes + pointer
    total_big_memory_in_KB =
        (double)((Total_Rules_in_Big_Node * 28) +
                 (Total_Tablesize_in_Big_Node * PTR_SIZE)) /
        1024;  // rule: 24 bytes + pointer
    total_memory_in_KB = total_tablesize_memory_in_KB +
                         total_linear_memory_in_KB + total_big_memory_in_KB;

    switch (num) {
      case 0:
        cout << ("\n\t***Set 0:***\n");
        break;
      case 1:
        cout << ("\n\t***Set 1:***\n");
        break;
      case 2:
        cout << ("\n\t***Set 2:***\n");
        break;
      case 3:
        cout << ("\n\t***Set 3:***\n");
        break;
    }

    cout << "\trules in set: " << numrules
         << ", rules in small node: " << Total_Rules_in_Linear_Node
         << ", rules in big node: " << Total_Rules_in_Big_Node << endl;

    cout << "\ttablesize: " << tablesize
         << ", NULL_Node_Count = " << NULL_Node_Count
         << ", Small_Node_Count = " << Small_Node_Count
         << ", Big_Node_Count = " << Big_Node_Count << endl;

    // 設置小數點後顯示 3 位數
    cout << fixed << setprecision(3);

    cout << "\tTable_memory: " << total_tablesize_memory_in_KB << "(KB), "
         << "Total_Rules_memory(Small): " << total_linear_memory_in_KB
         << "(KB), "
         << "Total_Rules_memory(Big): " << total_big_memory_in_KB << "(KB)"
         << endl;

    cout << "\tTotal memory: " << total_memory_in_KB << "(KB), "
         << "Byte/rule: " << (double(total_memory_in_KB * 1024) / numrules)
         << endl;
  }

 private:
  // // JIA
  // std::vector<SegmentNode> nodeKSetBuffer;  // 真正持有資料的 vector
  // SegmentNode* nodeKSet;                    // raw pointer，與原本程式相容

  SegmentNode* nodeKSet;

  size_t num;  // identify which sets
  size_t usedbit;
  size_t tablesize;
  size_t locate_segment;  // 32 - kset

  int numrules;
  size_t threshold;
  size_t threshold2;
  size_t bigSetPrefix;
  size_t bigSetPrefix3;

  size_t Total_Rules_in_Linear_Node;
  size_t Total_Rules_in_Big_Node;
  size_t Total_Tablesize_in_Big_Node;

  // count number of Small or Big segment in the set
  size_t NULL_Node_Count;
  size_t Small_Node_Count;
  size_t Big_Node_Count;

  double total_tablesize_memory_in_KB;
  double total_linear_memory_in_KB;
  double total_big_memory_in_KB;
  double total_memory_in_KB;
};

#endif
