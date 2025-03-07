#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "KSet.hpp"

using namespace std;

FILE *fpr;  // ruleset file
FILE *fpt;  //  trace file

// default do search, no update
int classificationFlag = 1;
int updateFlag = 0;
int threshold = 10;  // linear search threshold

// rdtscp
__inline__ uint64_t perf_counter(void) {
  uint32_t lo, hi;
  // take time stamp counter, rdtscp does serialize by itself, and is much
  // cheaper than using CPUID
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

// for KSet
int pre_K = 16;
int SetBits[3] = {8, 8, 8};
int max_pri_set[4] = {-1, -1, -1, -1};

int seed = 11;

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * loadrule(FILE *fp) Description:  load rules from rule file
 * =====================================================================================
 */
vector<Rule> loadrule(FILE *fp) {
  size_t i;
  uint32_t tmp;
  uint32_t max_uint = 0xFFFFFFFF;
  uint32_t mask;
  uint32_t sip1, sip2, sip3, sip4, smask;
  uint32_t dip1, dip2, dip3, dip4, dmask;
  uint32_t sport1, sport2;
  uint32_t dport1, dport2;
  uint32_t protocal, protocol_mask;
  uint32_t ht, htmask;
  size_t number_rule = 0;  // number of rules

  vector<Rule> rule;

  while (1) {
    Rule r;
    array<uint32_t, 2> points;
    if (fscanf(
            fp,
            "@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t%d : %d\t%d : %d\t%x/%x\t%x/%x\n",
            &sip1, &sip2, &sip3, &sip4, &smask, &dip1, &dip2, &dip3, &dip4,
            &dmask, &sport1, &sport2, &dport1, &dport2, &protocal,
            &protocol_mask, &ht, &htmask) != 18)
      break;

    if (smask == 0) {
      points[0] = 0;
      points[1] = 0xFFFFFFFF;
    } else if (smask > 0 && smask <= 8) {
      tmp = sip1 << 24;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 8 && smask <= 16) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 16 && smask <= 24) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      tmp += sip3 << 8;
      mask = ~(max_uint >> smask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else if (smask > 24 && smask <= 32) {
      tmp = sip1 << 24;
      tmp += sip2 << 16;
      tmp += sip3 << 8;
      tmp += sip4;
      mask = smask != 32 ? ~(max_uint >> smask) : max_uint;
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - smask)) - 1;
    } else {
      cout << ("Src IP length exceeds 32\n");
      exit(-1);
    }
    r.range[0] = points;

    if (dmask == 0) {
      points[0] = 0;
      points[1] = 0xFFFFFFFF;
    } else if (dmask > 0 && dmask <= 8) {
      tmp = dip1 << 24;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 8 && dmask <= 16) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 16 && dmask <= 24) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      tmp += dip3 << 8;
      mask = ~(max_uint >> dmask);
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else if (dmask > 24 && dmask <= 32) {
      tmp = dip1 << 24;
      tmp += dip2 << 16;
      tmp += dip3 << 8;
      tmp += dip4;
      mask = dmask != 32 ? ~(max_uint >> dmask) : max_uint;
      tmp &= mask;
      points[0] = tmp;
      points[1] = points[0] + (1 << (32 - dmask)) - 1;
    } else {
      cout << ("Dest IP length exceeds 32\n");
      exit(-1);
    }
    r.range[1] = points;

    points[0] = sport1;
    points[1] = sport2;
    r.range[2] = points;

    points[0] = dport1;
    points[1] = dport2;
    r.range[3] = points;

    if (protocol_mask == 0xFF) {
      points[0] = protocal;
      points[1] = protocal;
    } else if (protocol_mask == 0) {
      points[0] = 0;
      points[1] = 0xFF;
    } else {
      cout << ("Protocol mask error\n");
      exit(-1);
    }
    r.range[4] = points;

    r.prefix_length[0] = smask;
    r.prefix_length[1] = dmask;
    rule.push_back(r);
    ++number_rule;
  }

  // printf("the number of rules = %d\n", number_rule);
  size_t max_pri = number_rule - 1;
  for (i = 0; i < number_rule; ++i) {
    rule[i].priority = max_pri - i;
    /*printf("%u: %u:%u %u:%u %u:%u %u:%u %u:%u %d\n", i,
      rule[i].range[0][0], rule[i].range[0][1],
      rule[i].range[1][0], rule[i].range[1][1],
      rule[i].range[2][0], rule[i].range[2][1],
      rule[i].range[3][0], rule[i].range[3][1],
      rule[i].range[4][0], rule[i].range[4][1],
      rule[i].priority);*/
  }
  return rule;
}

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * loadpacket(FILE *fp) Description:  load packets from trace file generated
 * from ClassBench[INFOCOM2005]
 * =====================================================================================
 */
vector<Packet> loadpacket(FILE *fp) {
  unsigned int header[MAXDIMENSIONS];
  unsigned int proto_mask, fid;
  int number_pkt = 0;  // number of packets
  vector<Packet> packets;
  while (1) {
    if (fscanf(fp, "%u %u %u %u %u %u %u\n", &header[0], &header[1], &header[2],
               &header[3], &header[4], &proto_mask, &fid) == EOF)
      break;
    Packet p;
    p.push_back(header[0]);
    p.push_back(header[1]);
    p.push_back(header[2]);
    p.push_back(header[3]);
    p.push_back(header[4]);
    p.push_back(fid);

    packets.push_back(p);
    ++number_pkt;
  }
  /*
  printf("the number of packets = %d\n", number_pkt);
  for(size_t i=0;i<number_pkt;++i){
     printf("%u: %u %u %u %u %u %u\n", i,
     packets[i][0],
     packets[i][1],
     packets[i][2],
     packets[i][3],
     packets[i][4],
     packets[i][5]);
  }*/

  return packets;
}

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * parseargs(int argc, char *argv[]) Description:  Parse arguments from the
 * console
 * =====================================================================================
 */
void parseargs(int argc, char *argv[]) {
  int c;
  while ((c = getopt(argc, argv, "k:t:r:e:c:u:h")) != -1) {
    switch (c) {
      case 'k':
        // k = atoi(optarg);
        break;
      case 't':
        // seed = atoi(optarg);
        break;
      case 'r':  // rule set
        fpr = fopen(optarg, "r");
        break;
      case 'e':  // trace packets for simulations
        fpt = fopen(optarg, "r");
        break;
      case 'c':  // classification simulation
        classificationFlag = atoi(optarg);
        break;
      case 'u':  // update simulation
        updateFlag = atoi(optarg);
        break;
      case 'h':  // help
        printf(
            "./main [-k k][-r ruleset][-e trace][-c (1:classification)][-u "
            "(1:update)]\n");
        printf(
            "e.g., ./main -k 8 -r ./ruleset/ipc1_10k -e "
            "./ruleset/ipc1_10k_trace -c 1 -u 1\n");
        exit(1);
        break;
    }
  }
}

void anaK(const int number_rule, const vector<Rule> &rule, int *usedbits,
          vector<Rule> set[4], int k) {
  size_t i, j;

  uint32_t tmpKey;
  int hash;

  int pre_seg[4] = {0};
  int max[3] = {-1};

  // compute rules in each set
  for (i = 0; i < number_rule; ++i) {
    // Set 0
    if ((rule[i].prefix_length[0] >= k) && (rule[i].prefix_length[1] >= k)) {
      if (rule[i].priority > max_pri_set[0]) max_pri_set[0] = rule[i].priority;
      ++pre_seg[0];
    }
    // Set 1
    else if ((rule[i].prefix_length[0] >= k) &&
             (rule[i].prefix_length[1] < k)) {
      if (rule[i].priority > max_pri_set[1]) max_pri_set[1] = rule[i].priority;
      ++pre_seg[1];
    }
    // Set 2
    else if ((rule[i].prefix_length[0] < k) &&
             (rule[i].prefix_length[1] >= k)) {
      if (rule[i].priority > max_pri_set[2]) max_pri_set[2] = rule[i].priority;
      ++pre_seg[2];
    }
    // Set 3
    else {
      if (rule[i].priority > max_pri_set[3]) max_pri_set[3] = rule[i].priority;
      ++pre_seg[3];
    }
  }
  printf("================Precompute=============\n");
  printf("Set 0: %d, Set 1: %d, Set 2: %d, Set 3: %d\n", pre_seg[0], pre_seg[1],
         pre_seg[2], pre_seg[3]);
  printf("max_pri[0]: %d, max_pri[1]: %d, max_pri[2]: %d, max_pri[3]: %d\n",
         max_pri_set[0], max_pri_set[1], max_pri_set[2], max_pri_set[3]);
  // compute used bits
  for (i = 0; i < 3; ++i) {
    if (pre_seg[i] == 0) continue;
    if (8 < log2(pre_seg[i]) && log2(pre_seg[i]) < 16) {
      usedbits[i] = log2(pre_seg[i]);
    } else if (16 < log2(pre_seg[i])) {
      usedbits[i] = 16;
    } else {
      usedbits[i] = 8;
    }
  }
  printf("Set 0: %d bits, Set 1: %d bits, Set 2: %d bits\n", usedbits[0],
         usedbits[1], usedbits[2]);

  for (i = 0; i < 4; ++i) {
    max_pri_set[i] = -1;
  }
  // put rules in each set and analyze
  anaSet Set[3];
  for (i = 0; i < 3; ++i) {
    Set[i].tablesize = pow(2, usedbits[i]);
    Set[i].seg = new int[Set[i].tablesize];
  }

  for (i = 0; i < number_rule; ++i) {
    // Set 0
    if ((rule[i].prefix_length[0] >= usedbits[0]) &&
        (rule[i].prefix_length[1] >= usedbits[0])) {
      set[0].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[0]) max_pri_set[0] = rule[i].priority;
      // compute orig_key
      tmpKey = rule[i].range[0][0] >> (32 - usedbits[0]);
      tmpKey <<= usedbits[0];
      tmpKey += (rule[i].range[1][0] >> (32 - usedbits[0]));
      // compute hash to locate seg
      hash = hashSet0(tmpKey, usedbits[0]);

      if (Set[0].seg[hash] == 0) {  // empty
        ++Set[0].non_empty_seg;
      }
      ++Set[0].seg[hash];
    }
    // Set 1
    else if (rule[i].prefix_length[0] >= usedbits[1]) {
      set[1].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[1]) max_pri_set[1] = rule[i].priority;
      tmpKey = rule[i].range[0][0] >> (32 - usedbits[1]);

      if (Set[1].seg[tmpKey] == 0) {  // empty
        ++Set[1].non_empty_seg;
      }
      ++Set[1].seg[tmpKey];
    }
    // Set 2
    else if (rule[i].prefix_length[1] >= usedbits[2]) {
      set[2].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[2]) max_pri_set[2] = rule[i].priority;
      tmpKey = rule[i].range[1][0] >> (32 - usedbits[2]);

      if (Set[2].seg[tmpKey] == 0) {  // empty
        ++Set[2].non_empty_seg;
      }
      ++Set[2].seg[tmpKey];
    }
    // Set 3
    else {
      set[3].push_back(rule[i]);
      if (rule[i].priority > max_pri_set[3]) max_pri_set[3] = rule[i].priority;
    }
  }

  // max segment
  for (i = 0; i < 3; ++i) {
    for (j = 0; j < Set[i].tablesize; ++j) {
      if (max[i] < Set[i].seg[j]) max[i] = Set[i].seg[j];
    }
  }

  printf("================Compute=============\n");
  printf("Set 0: %d, Set 1: %d, Set 2: %d, Set 3: %d\n", set[0].size(),
         set[1].size(), set[2].size(), set[3].size());
  printf("max_pri[0]: %d, max_pri[1]: %d, max_pri[2]: %d, max_pri[3]: %d\n",
         max_pri_set[0], max_pri_set[1], max_pri_set[2], max_pri_set[3]);
  printf(
      "non-empty_seg[0] = %d, non-empty_seg[1] = %d, non-empty_seg[2] = %d\n",
      Set[0].non_empty_seg, Set[1].non_empty_seg, Set[2].non_empty_seg);
  printf("AVG[0]: %.3f, AVG[1]: %.3f, AVG[2]: %.3f\n",
         (float)set[0].size() / Set[0].non_empty_seg,
         (float)set[1].size() / Set[1].non_empty_seg,
         (float)set[2].size() / Set[2].non_empty_seg);
  printf("MAX[0]: %d, MAX[1]: %d, MAX[2]: %d\n", max[0], max[1], max[2]);
}

int main(int argc, char *argv[]) {
  parseargs(argc, argv);
  vector<Rule> rule, build, update;
  vector<Packet> packets;
  vector<int> results;
  size_t i, j, t;
  int number_rule = 0;
  int number_build_rule = 0;
  int number_update_rule = 0;
  vector<Rule> set_4[4];

  // rdtscp
  uint64_t t1, t2, t3, t4;
  double time_rdtscp, time_rdtscp2, time_search;

  if (fpr != NULL) {
    rule = loadrule(fpr);

    number_rule = rule.size();
    printf("The number of rules = %d\n", number_rule);

    if (updateFlag == 1) {
      srand(seed);
      int randn;

      for (size_t i = 0; i < number_rule; ++i) {
        randn = rand() % 100;
        if (randn < 95)
          build.push_back(rule[i]);
        else
          update.push_back(rule[i]);
      }

      number_build_rule = build.size();
      number_update_rule = update.size();

      printf("The number of rules for build = %d\n", number_build_rule);

      anaK(number_build_rule, build, SetBits, set_4, pre_K);
    } else {
      anaK(number_rule, rule, SetBits, set_4, pre_K);
    }

    int num_set[4] = {0};
    for (i = 0; i < 4; ++i) {
      num_set[i] = set_4[i].size();
    }

    KSet set0(0, set_4[0], SetBits[0]);
    KSet set1(1, set_4[1], SetBits[1]);
    KSet set2(2, set_4[2], SetBits[2]);
    KSet set3(3, set_4[3], 0);
    // construct
    printf("**************** Construction ****************\n");
    t1 = perf_counter();
    if (num_set[0] > 0) {
      set0.ConstructClassifier(set_4[0]);
    }
    if (num_set[1] > 0) {
      set1.ConstructClassifier(set_4[1]);
    }
    if (num_set[2] > 0) {
      set2.ConstructClassifier(set_4[2]);
    }
    if (num_set[3] > 0) {
      set3.ConstructClassifier(set_4[3]);
    }
    t2 = perf_counter();
    time_rdtscp = (double)(t2 - t1) / 1e3 / 5000;

    printf("\tConstruction time: %.3f ms\n", time_rdtscp);

    // print memory
    set0.prints();
    set1.prints();
    set2.prints();
    set3.prints();

    if (classificationFlag == 1) {
      // classification
      printf("\n**************** Classification ****************\n");
      packets = loadpacket(fpt);

      int number_pkt = packets.size();
      printf("\tThe number of packet in the trace file = %d\n", number_pkt);
      const int trials = 10;  // run 10 times circularly
      printf("\tTotal packets (run %d times circularly): %d\n", trials,
             packets.size() * trials);
      int match_miss = 0;

      int match_pri = -2;
      vector<int> matchid(number_pkt);
      Packet p;

      if (max_pri_set[1] < max_pri_set[2]) max_pri_set[1] = max_pri_set[2];

      for (t = 0; t < trials; ++t) {
        t1 = perf_counter();
        for (i = 0; i < number_pkt; ++i) {
          p = packets[i];
          match_pri = -1;
          if (num_set[0] > 0) match_pri = set0.ClassifyAPacket(p);
          if (match_pri < max_pri_set[1] && num_set[1] > 0)
            match_pri = max(match_pri, set1.ClassifyAPacket(p));
          if (match_pri < max_pri_set[2] && num_set[2] > 0)
            match_pri = max(match_pri, set2.ClassifyAPacket(p));
          if (match_pri < max_pri_set[3] && num_set[3] > 0)
            match_pri = max(match_pri, set3.ClassifyAPacket(p));
          matchid[i] = number_rule - 1 - match_pri;
        }
        t2 = perf_counter();
        time_rdtscp += (double)(t2 - t1) / 5000;
        for (i = 0; i < number_pkt; ++i) {
          if (matchid[i] == -1)
            ++match_miss;
          else if (packets[i][5] < matchid[i])
            ++match_miss;  // > is correct: match a rule with a higher priority
                           // (i.e., smaller rule id)
        }
      }
      printf("\t%d packets are classified, %d of them are misclassified\n",
             number_pkt * trials, match_miss);
      printf("\tTotal classification time: %.3f s\n",
             time_rdtscp / 1e6 / trials);
      printf("\tAverage classification time: %.3f us\n",
             time_rdtscp / (trials * packets.size()));
      printf("\tThroughput: %.3f Mpps\n",
             1 / (time_rdtscp / (trials * packets.size())));
    }

    if (updateFlag == 1) {
      printf("\n**************** Update ****************\n");
      printf("\tThe number of updated rules = %d\n", number_update_rule);
      int srcmask, dstmask;

      t1 = perf_counter();
      // insert
      for (size_t i = 0; i < number_update_rule; ++i) {
        srcmask = update[i].prefix_length[0];
        dstmask = update[i].prefix_length[1];
        if ((srcmask >= SetBits[0]) && (dstmask >= SetBits[0]))
          set0.InsertRule(update[i]);
        else if (srcmask >= SetBits[1])
          set1.InsertRule(update[i]);
        else if (dstmask >= SetBits[2])
          set2.InsertRule(update[i]);
        else
          set3.InsertRule(update[i]);
      }
      t2 = perf_counter();

      // delete
      for (size_t i = 0; i < number_update_rule; ++i) {
        srcmask = update[i].prefix_length[0];
        dstmask = update[i].prefix_length[1];
        if ((srcmask >= SetBits[0]) && (dstmask >= SetBits[0]))
          set0.DeleteRule(update[i]);
        else if (srcmask >= SetBits[1])
          set1.DeleteRule(update[i]);
        else if (dstmask >= SetBits[2])
          set2.DeleteRule(update[i]);
        else
          set3.DeleteRule(update[i]);
      }
      t3 = perf_counter();
      time_rdtscp = (double)(t2 - t1) / 5000;
      time_rdtscp2 = (double)(t3 - t2) / 5000;
      printf("\tInsert time = %.3f s, delete time = %.3f s\n",
             time_rdtscp / 1e6, time_rdtscp2 / 1e6);
      printf("\tTotal update time: %.3f s\n",
             (time_rdtscp + time_rdtscp2) / 1e6);
      printf("\tAverage insert time: %.3f us, Average delete time: %.3f us\n",
             time_rdtscp / number_update_rule,
             time_rdtscp2 / number_update_rule);
      printf("\tAverage update time: %.3f us\n",
             (time_rdtscp + time_rdtscp2) / (number_update_rule * 2));
    }
  }

  return 0;
}
