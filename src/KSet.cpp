#include "KSet.hpp"
using namespace std;

KSet::KSet(int num1, vector<Rule> &classifier, int usedbits) {
  num = num1;
  usedbit = usedbits;
  locate_segment = 32 - usedbits;

  tablesize = pow(2, usedbits);

  numrules = classifier.size();
  threshold = 10;
  threshold2 = 4;
  bigSetPrefix = 30;
  bigSetPrefix3 = 7;

  // // JIA
  // nodeKSetBuffer.resize(tablesize);
  // nodeKSet = nodeKSetBuffer.data();
  nodeKSet = new SegmentNode[tablesize];

  Total_Rules_in_Linear_Node = 0;
  Total_Rules_in_Big_Node = 0;
  Total_Tablesize_in_Big_Node = 0;

  // count number of linear or PSTSS segment in the set
  NULL_Node_Count = 0;
  Small_Node_Count = 0;
  Big_Node_Count = 0;

  total_linear_memory_in_KB = 0;
  total_big_memory_in_KB = 0;
  total_memory_in_KB = 0;
}

KSet::~KSet() { delete[] nodeKSet; /* JIA */ }

// return the used table size
int simple_part(const vector<Rule> &rules, BigSegment *b, vector<Rule> &rmd,
                int bits, int t) {
  size_t i, j;
  uint32_t v;
  int hash;
  int r = 0;

  for (i = 0; i < rules.size(); ++i) {
    if (rules[i].prefix_length[0] >= bits) {  // SrcIP
      b[0].list.push_back(rules[i]);
    } else if (rules[i].prefix_length[1] >= bits) {  // DstIP
      b[1].list.push_back(rules[i]);
    } else if (rules[i].range[3][0] == rules[i].range[3][1]) {  // DstPort
      b[2].list.push_back(rules[i]);
    } else if (rules[i].range[2][0] == rules[i].range[2][1]) {  // SrcPort
      b[3].list.push_back(rules[i]);
    } else {
      rmd.push_back(rules[i]);
    }
  }
  rmd.shrink_to_fit();

  for (i = 0; i < 4; ++i) {
    if (b[i].list.size() == 0) {
      b[i].bf = -1;
      b[i].uf = -1;
    } else if (b[i].list.size() <= t) {  // small, only list
                                         // else {
      b[i].bf = 0;
      b[i].uf = part_oder(i);
      ++r;
      b[i].list.shrink_to_fit();
    } else {  // big, need build hash
      b[i].bf = 1;
      b[i].uf = part_oder(i);

      b[i].ub = log2(b[i].list.size());
      b[i].ht_size = pow(2, b[i].ub);
      r += b[i].ht_size;
      b[i].ht = new vector<Rule>[b[i].ht_size];

      for (j = 0; j < b[i].list.size(); ++j) {
        v = b[i].list[j].range[b[i].uf][0];
        if ((i == 0) || (i == 1)) {
          v >>= (32 - bits);
        }
        hash = exactHash(v, b[i].ub);
        b[i].ht[hash].push_back(b[i].list[j]);
      }
    }
  }

  return r;
}

void KSet::ConstructClassifier(const vector<Rule> &rules) {
  uint32_t value = 0;
  size_t i, j;

  // allocate segment
  for (i = 0; i < rules.size(); ++i) {
    switch (num) {
      case 0:
        value = rules[i].range[0][0] >> locate_segment;
        value <<= usedbit;
        value += (rules[i].range[1][0] >> locate_segment);
        value = hashSet0(value, usedbit);
        break;
      case 1:
        value = rules[i].range[0][0] >> locate_segment;
        break;
      case 2:
        value = rules[i].range[1][0] >> locate_segment;
        break;
      case 3:
        value = 0;
        break;
      default:
        cout << ("not suitable case.\n");
    }
    nodeKSet[value].classifier.push_back(rules[i]);
    ++nodeKSet[value].nrules;
    nodeKSet[value].classifier.shrink_to_fit();
  }
  for (i = 0; i < tablesize; ++i) {
    if (nodeKSet[i].nrules == 0) {
      ++NULL_Node_Count;
    } else if (nodeKSet[i].nrules <= threshold) {  // small segment
      nodeKSet[i].flag = 0;
      ++Small_Node_Count;
      Total_Rules_in_Linear_Node += nodeKSet[i].nrules;
    } else {  // big segment
      nodeKSet[i].flag = 1;
      ++Big_Node_Count;
      Total_Rules_in_Big_Node += nodeKSet[i].nrules;

      if (num != 3) {
        Total_Tablesize_in_Big_Node +=
            simple_part(nodeKSet[i].classifier, nodeKSet[i].b, nodeKSet[i].rmd,
                        bigSetPrefix, threshold2);
      } else {
        Total_Tablesize_in_Big_Node +=
            simple_part(nodeKSet[i].classifier, nodeKSet[i].b, nodeKSet[i].rmd,
                        bigSetPrefix3, threshold2);
      }
      // check rmd
      if (nodeKSet[i].rmd.size() == 0) {
        nodeKSet[i].r_flag = -1;
      } else if (nodeKSet[i].rmd.size() <= threshold2) {  // linear
        nodeKSet[i].r_flag = 0;
        ++Total_Tablesize_in_Big_Node;
      } else {  // tcp, udp, other
        nodeKSet[i].r_flag = 1;
        Total_Tablesize_in_Big_Node += 3;
        for (j = 0; j < nodeKSet[i].rmd.size(); ++j) {
          if (nodeKSet[i].r_max_pri < nodeKSet[i].rmd[j].priority)
            nodeKSet[i].r_max_pri = nodeKSet[i].rmd[j].priority;
          if (nodeKSet[i].rmd[j].range[4][0] == 6) {  // tcp
            nodeKSet[i].rmdp[0].push_back(nodeKSet[i].rmd[j]);
            if (nodeKSet[i].rp_max_pri[0] < nodeKSet[i].rmd[j].priority)
              nodeKSet[i].rp_max_pri[0] = nodeKSet[i].rmd[j].priority;
          } else if (nodeKSet[i].rmd[j].range[4][0] == 17) {  // udp
            nodeKSet[i].rmdp[1].push_back(nodeKSet[i].rmd[j]);
            if (nodeKSet[i].rp_max_pri[1] < nodeKSet[i].rmd[j].priority)
              nodeKSet[i].rp_max_pri[1] = nodeKSet[i].rmd[j].priority;
          } else {
            nodeKSet[i].rmdp[2].push_back(nodeKSet[i].rmd[j]);
            if (nodeKSet[i].rp_max_pri[2] < nodeKSet[i].rmd[j].priority)
              nodeKSet[i].rp_max_pri[2] = nodeKSet[i].rmd[j].priority;
          }
        }
      }
    }
  }
}

int linear_search(const Packet &packet, const vector<Rule> &rules) {
  int match_id = -1;

  for (size_t i = 0; i < rules.size(); ++i) {
    if ((packet[0] >= rules[i].range[0][0]) &&
        (packet[0] <= rules[i].range[0][1]) &&
        (packet[1] >= rules[i].range[1][0]) &&
        (packet[1] <= rules[i].range[1][1]) &&
        (packet[2] >= rules[i].range[2][0]) &&
        (packet[2] <= rules[i].range[2][1]) &&
        (packet[3] >= rules[i].range[3][0]) &&
        (packet[3] <= rules[i].range[3][1]) &&
        (packet[4] >= rules[i].range[4][0]) &&
        (packet[4] <= rules[i].range[4][1])) {
      match_id = rules[i].priority;
      break;
    }
  }

  return match_id;
}

int KSet::ClassifyAPacket(const Packet &packet) {
  size_t i;
  uint32_t value = 0;
  uint32_t v2;
  int h2;
  int match_id = -1;

  // find segment node
  switch (num) {
    case 0:
      value = packet[0] >> locate_segment;
      value <<= usedbit;
      value += (packet[1] >> locate_segment);
      value = hashSet0(value, usedbit);
      break;
    case 1:
      value = packet[0] >> locate_segment;
      break;
    case 2:
      value = packet[1] >> locate_segment;
      break;
    case 3:
      value = 0;
      break;
    default:
      cout << ("not suitable case.\n");
  }

  if (nodeKSet[value].flag == 0) {  // small
    match_id = linear_search(packet, nodeKSet[value].classifier);
  } else if (nodeKSet[value].flag == 1) {  // big

    for (i = 0; i < 4; ++i) {
      if (nodeKSet[value].b[i].bf == 0) {  // small list
        match_id =
            max(match_id, linear_search(packet, nodeKSet[value].b[i].list));
      } else if (nodeKSet[value].b[i].bf == 1) {  // big hash
        // check used field, then do hash
        v2 = packet[nodeKSet[value].b[i].uf];
        if ((i == 0) || (i == 1)) {
          if (num != 3) {
            v2 >>= (32 - bigSetPrefix);
          } else {
            v2 >>= (32 - bigSetPrefix3);
          }
        }

        h2 = exactHash(v2, nodeKSet[value].b[i].ub);
        match_id =
            max(match_id, linear_search(packet, nodeKSet[value].b[i].ht[h2]));
      }
    }
    // search remainder
    if (nodeKSet[value].r_flag == 0) {
      if (nodeKSet[value].r_max_pri > match_id) {
        match_id = max(match_id, linear_search(packet, nodeKSet[value].rmd));
      }
    } else if (nodeKSet[value].r_flag == 1) {
      if (packet[4] == 6) {  // tcp
        if (nodeKSet[value].rp_max_pri[0] > match_id) {
          match_id =
              max(match_id, linear_search(packet, nodeKSet[value].rmdp[0]));
        }
      } else if (packet[4] == 17) {  // udp
        if (nodeKSet[value].rp_max_pri[1] > match_id) {
          match_id =
              max(match_id, linear_search(packet, nodeKSet[value].rmdp[1]));
        }
      }
      // remainder 2
      if (nodeKSet[value].rp_max_pri[2] > match_id) {
        match_id =
            max(match_id, linear_search(packet, nodeKSet[value].rmdp[2]));
      }
    }
  }

  return match_id;
}

void linear_delete(const Rule &delete_rule, vector<Rule> &rules, int *nrules,
                   int *numrules) {
  size_t i, j;
  int del_flag = 1;  // default insert

  for (i = 0; i < rules.size(); ++i) {
    if (rules[i].priority == delete_rule.priority &&
        rules[i].range[0][0] == delete_rule.range[0][0] &&
        rules[i].range[0][1] == delete_rule.range[0][1] &&
        rules[i].range[1][0] == delete_rule.range[1][0] &&
        rules[i].range[1][1] == delete_rule.range[1][1] &&
        rules[i].range[2][0] == delete_rule.range[2][0] &&
        rules[i].range[2][1] == delete_rule.range[2][1] &&
        rules[i].range[3][0] == delete_rule.range[3][0] &&
        rules[i].range[3][1] == delete_rule.range[3][1] &&
        rules[i].range[4][0] == delete_rule.range[4][0] &&
        rules[i].range[4][1] == delete_rule.range[4][1])
      break;
  }
  if (i == (rules.size() - 1)) {  // not found
    del_flag = 0;
  }

  if (del_flag == 1) {
    for (j = i; j < (rules.size() - 1); ++j) {
      rules[j] = rules[j + 1];
    }
    rules.pop_back();
    (*nrules)--;
    (*numrules)--;
  }
}

void KSet::DeleteRule(const Rule &delete_rule) {
  uint32_t value = 0;
  size_t big_bits;
  uint32_t v2;
  int h2;

  // find segment node
  switch (num) {
    case 0:
      value = delete_rule.range[0][0] >> locate_segment;
      value <<= usedbit;
      value += (delete_rule.range[1][0] >> locate_segment);
      value = hashSet0(value, usedbit);
      break;
    case 1:
      value = delete_rule.range[0][0] >> locate_segment;
      break;
    case 2:
      value = delete_rule.range[1][0] >> locate_segment;
      break;
    case 3:
      value = 0;
      break;
    default:
      cout << ("not suitable case.\n");
  }

  if (nodeKSet[value].flag == 0) {  // small
    if (nodeKSet[value].nrules > 0) {
      linear_delete(delete_rule, nodeKSet[value].classifier,
                    &nodeKSet[value].nrules, &numrules);
    }
  } else if (nodeKSet[value].flag == 1) {  // big
    if (nodeKSet[value].nrules > 0) {
      if (num != 3) {
        big_bits = bigSetPrefix;
      } else {
        big_bits = bigSetPrefix3;
      }
      // find which b[]
      if (delete_rule.prefix_length[0] >= big_bits) {  // SrcIP
        if (nodeKSet[value].b[0].bf == 0) {            // list
          linear_delete(delete_rule, nodeKSet[value].b[0].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[0].bf == 1) {  // hash table
          v2 = delete_rule.range[0][0] >> (32 - big_bits);
          h2 = exactHash(v2, nodeKSet[value].b[0].ub);
          linear_delete(delete_rule, nodeKSet[value].b[0].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (delete_rule.prefix_length[1] >= big_bits) {  // DstIP
        if (nodeKSet[value].b[1].bf == 0) {                   // list
          linear_delete(delete_rule, nodeKSet[value].b[1].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[1].bf == 1) {  // hash table
          v2 = delete_rule.range[1][0] >> (32 - big_bits);
          h2 = exactHash(v2, nodeKSet[value].b[1].ub);
          linear_delete(delete_rule, nodeKSet[value].b[1].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (delete_rule.range[3][0] ==
                 delete_rule.range[3][1]) {  // DstPort
        if (nodeKSet[value].b[2].bf == 0) {  // list
          linear_delete(delete_rule, nodeKSet[value].b[2].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[2].bf == 1) {  // hash table
          v2 = delete_rule.range[3][0];
          h2 = exactHash(v2, nodeKSet[value].b[2].ub);
          linear_delete(delete_rule, nodeKSet[value].b[2].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (delete_rule.range[2][0] ==
                 delete_rule.range[2][1]) {  // SrcPort
        if (nodeKSet[value].b[3].bf == 0) {  // list
          linear_delete(delete_rule, nodeKSet[value].b[3].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[3].bf == 1) {  // hash table
          v2 = delete_rule.range[2][0];
          h2 = exactHash(v2, nodeKSet[value].b[3].ub);
          linear_delete(delete_rule, nodeKSet[value].b[3].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        }
      } else {  // rmd
        if (nodeKSet[value].r_flag == 0) {
          linear_delete(delete_rule, nodeKSet[value].rmd,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].r_flag == 1) {
          if (delete_rule.range[4][0] == 6) {  // tcp
            linear_delete(delete_rule, nodeKSet[value].rmdp[0],
                          &nodeKSet[value].nrules, &numrules);
          } else if (delete_rule.range[4][0] == 17) {  // udp
            linear_delete(delete_rule, nodeKSet[value].rmdp[1],
                          &nodeKSet[value].nrules, &numrules);
          } else {  // other
            linear_delete(delete_rule, nodeKSet[value].rmdp[2],
                          &nodeKSet[value].nrules, &numrules);
          }
        }
      }
    }
  } else if (nodeKSet[value].flag == -1) {
    cout << "num = " << num << ", the segment is empty, delete failed! rule id="
         << delete_rule.priority << endl;
  }
}

void linear_insert(const Rule &insert_rule, vector<Rule> &rules, int *nrules,
                   int *numrules) {
  size_t i;
  bool ins_flag = true;  // default insert

  // if alreadly exist
  for (i = 0; i < rules.size(); ++i) {
    if (rules[i].priority == insert_rule.priority &&
        rules[i].range[0][0] == insert_rule.range[0][0] &&
        rules[i].range[0][1] == insert_rule.range[0][1] &&
        rules[i].range[1][0] == insert_rule.range[1][0] &&
        rules[i].range[1][1] == insert_rule.range[1][1] &&
        rules[i].range[2][0] == insert_rule.range[2][0] &&
        rules[i].range[2][1] == insert_rule.range[2][1] &&
        rules[i].range[3][0] == insert_rule.range[3][0] &&
        rules[i].range[3][1] == insert_rule.range[3][1] &&
        rules[i].range[4][0] == insert_rule.range[4][0] &&
        rules[i].range[4][1] == insert_rule.range[4][1]) {
      ins_flag = false;
      break;
    }
  }
  if (ins_flag) {
    rules.insert(upper_bound(rules.begin(), rules.end(), insert_rule, cmpp),
                 insert_rule);
    ++(*nrules);
    ++(*numrules);
  }
}

void KSet::InsertRule(const Rule &insert_rule) {
  uint32_t value = 0;
  size_t big_bits;
  uint32_t v2;
  int h2;

  // find segment node
  switch (num) {
    case 0:
      value = insert_rule.range[0][0] >> locate_segment;
      value <<= usedbit;
      value += (insert_rule.range[1][0] >> locate_segment);
      value = hashSet0(value, usedbit);
      break;
    case 1:
      value = insert_rule.range[0][0] >> locate_segment;
      break;
    case 2:
      value = insert_rule.range[1][0] >> locate_segment;
      break;
    case 3:
      value = 0;
      break;
    default:
      cout << ("not suitable case.\n");
  }

  if (nodeKSet[value].flag == 0) {  // small
    if (nodeKSet[value].nrules > 0) {
      linear_insert(insert_rule, nodeKSet[value].classifier,
                    &nodeKSet[value].nrules, &numrules);
    }
  } else if (nodeKSet[value].flag == 1) {  // big
    if (nodeKSet[value].nrules > 0) {
      if (num != 3) {
        big_bits = bigSetPrefix;
      } else {
        big_bits = bigSetPrefix3;
      }
      // find which b[]
      if (insert_rule.prefix_length[0] >= big_bits) {  // SrcIP
        if (nodeKSet[value].b[0].bf == 0) {            // list
          linear_insert(insert_rule, nodeKSet[value].b[0].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[0].bf == 1) {  // hash table
          v2 = insert_rule.range[0][0] >> (32 - big_bits);
          h2 = exactHash(v2, nodeKSet[value].b[0].ub);
          linear_insert(insert_rule, nodeKSet[value].b[0].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[0].bf == -1) {  // no space create for list
          nodeKSet[value].b[0].bf = 0;
          nodeKSet[value].b[0].uf = 0;
          linear_insert(insert_rule, nodeKSet[value].b[0].list,
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (insert_rule.prefix_length[1] >= big_bits) {  // DstIP
        if (nodeKSet[value].b[1].bf == 0) {                   // list
          linear_insert(insert_rule, nodeKSet[value].b[1].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[1].bf == 1) {  // hash table
          v2 = insert_rule.range[1][0] >> (32 - big_bits);
          h2 = exactHash(v2, nodeKSet[value].b[1].ub);
          linear_insert(insert_rule, nodeKSet[value].b[1].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[1].bf == -1) {  // no space create for list
          nodeKSet[value].b[1].bf = 0;
          nodeKSet[value].b[1].uf = 1;
          linear_insert(insert_rule, nodeKSet[value].b[1].list,
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (insert_rule.range[3][0] ==
                 insert_rule.range[3][1]) {  // DstPort
        if (nodeKSet[value].b[2].bf == 0) {  // list
          linear_insert(insert_rule, nodeKSet[value].b[2].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[2].bf == 1) {  // hash table
          v2 = insert_rule.range[3][0];
          h2 = exactHash(v2, nodeKSet[value].b[2].ub);
          linear_insert(insert_rule, nodeKSet[value].b[2].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[2].bf == -1) {  // no space create for list
          nodeKSet[value].b[2].bf = 0;
          nodeKSet[value].b[2].uf = 3;
          linear_insert(insert_rule, nodeKSet[value].b[2].list,
                        &nodeKSet[value].nrules, &numrules);
        }
      } else if (insert_rule.range[2][0] ==
                 insert_rule.range[2][1]) {  // SrcPort
        if (nodeKSet[value].b[3].bf == 0) {  // list
          linear_insert(insert_rule, nodeKSet[value].b[3].list,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[3].bf == 1) {  // hash table
          v2 = insert_rule.range[2][0];
          h2 = exactHash(v2, nodeKSet[value].b[3].ub);
          linear_insert(insert_rule, nodeKSet[value].b[3].ht[h2],
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].b[3].bf == -1) {  // no space create for list
          nodeKSet[value].b[3].bf = 0;
          nodeKSet[value].b[3].uf = 2;
          linear_insert(insert_rule, nodeKSet[value].b[3].list,
                        &nodeKSet[value].nrules, &numrules);
        }
      } else {  // rmd
        if (nodeKSet[value].r_flag == 0) {
          linear_insert(insert_rule, nodeKSet[value].rmd,
                        &nodeKSet[value].nrules, &numrules);
        } else if (nodeKSet[value].r_flag == 1) {
          if (insert_rule.range[4][0] == 6) {  // tcp
            linear_insert(insert_rule, nodeKSet[value].rmdp[0],
                          &nodeKSet[value].nrules, &numrules);
          } else if (insert_rule.range[4][0] == 17) {  // udp
            linear_insert(insert_rule, nodeKSet[value].rmdp[1],
                          &nodeKSet[value].nrules, &numrules);
          } else {  // other
            linear_insert(insert_rule, nodeKSet[value].rmdp[2],
                          &nodeKSet[value].nrules, &numrules);
          }
        } else if (nodeKSet[value].r_flag == -1) {  // no space create for list
          nodeKSet[value].r_flag = 0;
          linear_insert(insert_rule, nodeKSet[value].rmd,
                        &nodeKSet[value].nrules, &numrules);
        }
      }
    }
  } else if (nodeKSet[value].flag == -1) {
    nodeKSet[value].flag = 0;
    ++nodeKSet[value].nrules;
    nodeKSet[value].classifier.push_back(insert_rule);
    nodeKSet[value].classifier.shrink_to_fit();
    nodeKSet[value].classifier.insert(
        upper_bound(nodeKSet[value].classifier.begin(),
                    nodeKSet[value].classifier.end(), insert_rule, cmpp),
        insert_rule);
    ++numrules;
  }
}
KSet::KSet(const KSet &other)
    : num(other.num),
      usedbit(other.usedbit),
      locate_segment(other.locate_segment),
      tablesize(other.tablesize),
      numrules(other.numrules),
      threshold(other.threshold),
      threshold2(other.threshold2),
      bigSetPrefix(other.bigSetPrefix),
      bigSetPrefix3(other.bigSetPrefix3),
      Total_Rules_in_Linear_Node(other.Total_Rules_in_Linear_Node),
      Total_Rules_in_Big_Node(other.Total_Rules_in_Big_Node),
      Total_Tablesize_in_Big_Node(other.Total_Tablesize_in_Big_Node),
      NULL_Node_Count(other.NULL_Node_Count),
      Small_Node_Count(other.Small_Node_Count),
      Big_Node_Count(other.Big_Node_Count),
      total_tablesize_memory_in_KB(other.total_tablesize_memory_in_KB),
      total_linear_memory_in_KB(other.total_linear_memory_in_KB),
      total_big_memory_in_KB(other.total_big_memory_in_KB),
      total_memory_in_KB(other.total_memory_in_KB) {
  nodeKSet = new SegmentNode[tablesize];
  std::copy(other.nodeKSet, other.nodeKSet + tablesize, nodeKSet);
}
KSet &KSet::operator=(const KSet &other) {
  if (this != &other) {
    delete[] nodeKSet;
    tablesize = other.tablesize;
    nodeKSet = new SegmentNode[tablesize];
    std::copy(other.nodeKSet, other.nodeKSet + tablesize, nodeKSet);

    // 其餘成員照抄
    num = other.num;
    usedbit = other.usedbit;
    locate_segment = other.locate_segment;
    numrules = other.numrules;
    threshold = other.threshold;
    threshold2 = other.threshold2;
    bigSetPrefix = other.bigSetPrefix;
    bigSetPrefix3 = other.bigSetPrefix3;
    Total_Rules_in_Linear_Node = other.Total_Rules_in_Linear_Node;
    Total_Rules_in_Big_Node = other.Total_Rules_in_Big_Node;
    Total_Tablesize_in_Big_Node = other.Total_Tablesize_in_Big_Node;
    NULL_Node_Count = other.NULL_Node_Count;
    Small_Node_Count = other.Small_Node_Count;
    Big_Node_Count = other.Big_Node_Count;
    total_tablesize_memory_in_KB = other.total_tablesize_memory_in_KB;
    total_linear_memory_in_KB = other.total_linear_memory_in_KB;
    total_big_memory_in_KB = other.total_big_memory_in_KB;
    total_memory_in_KB = other.total_memory_in_KB;
  }
  return *this;
}
KSet::KSet(KSet &&other) noexcept
    : nodeKSet(other.nodeKSet),
      num(other.num),
      usedbit(other.usedbit),
      locate_segment(other.locate_segment),
      tablesize(other.tablesize),
      numrules(other.numrules),
      threshold(other.threshold),
      threshold2(other.threshold2),
      bigSetPrefix(other.bigSetPrefix),
      bigSetPrefix3(other.bigSetPrefix3),
      Total_Rules_in_Linear_Node(other.Total_Rules_in_Linear_Node),
      Total_Rules_in_Big_Node(other.Total_Rules_in_Big_Node),
      Total_Tablesize_in_Big_Node(other.Total_Tablesize_in_Big_Node),
      NULL_Node_Count(other.NULL_Node_Count),
      Small_Node_Count(other.Small_Node_Count),
      Big_Node_Count(other.Big_Node_Count),
      total_tablesize_memory_in_KB(other.total_tablesize_memory_in_KB),
      total_linear_memory_in_KB(other.total_linear_memory_in_KB),
      total_big_memory_in_KB(other.total_big_memory_in_KB),
      total_memory_in_KB(other.total_memory_in_KB) {
  other.nodeKSet = nullptr;
}
KSet &KSet::operator=(KSet &&other) noexcept {
  if (this != &other) {
    delete[] nodeKSet;
    nodeKSet = other.nodeKSet;
    tablesize = other.tablesize;

    num = other.num;
    usedbit = other.usedbit;
    locate_segment = other.locate_segment;
    numrules = other.numrules;
    threshold = other.threshold;
    threshold2 = other.threshold2;
    bigSetPrefix = other.bigSetPrefix;
    bigSetPrefix3 = other.bigSetPrefix3;
    Total_Rules_in_Linear_Node = other.Total_Rules_in_Linear_Node;
    Total_Rules_in_Big_Node = other.Total_Rules_in_Big_Node;
    Total_Tablesize_in_Big_Node = other.Total_Tablesize_in_Big_Node;
    NULL_Node_Count = other.NULL_Node_Count;
    Small_Node_Count = other.Small_Node_Count;
    Big_Node_Count = other.Big_Node_Count;
    total_tablesize_memory_in_KB = other.total_tablesize_memory_in_KB;
    total_linear_memory_in_KB = other.total_linear_memory_in_KB;
    total_big_memory_in_KB = other.total_big_memory_in_KB;
    total_memory_in_KB = other.total_memory_in_KB;

    other.nodeKSet = nullptr;
  }
  return *this;
}
