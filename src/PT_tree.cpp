#include "PT_tree.hpp"

namespace PT {
uint8_t maskHash[33][4];
uint32_t maskBit[33] = {
    0,          0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000,
    0xFC000000, 0xFE000000, 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
    0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000,
    0xFFFFC000, 0xFFFFE000, 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
    0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0, 0xFFFFFFF8,
    0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF};

/// === ///
int check_correct(PT_Rule& a, PT_Packet& b) {
  if (a.protocol[0] != 0 && (uint32_t)a.protocol[1] != b.protocol) return 0;
  int mask = 32 - (uint32_t)a.source_mask;
  uint32_t sip, dip;
  memcpy(&sip, a.source_ip, 4);
  memcpy(&dip, b.source_ip, 4);
  if (mask != 32 && (sip >> mask) != (dip >> mask)) return 0;
  mask = 32 - (uint32_t)a.destination_mask;
  memcpy(&sip, a.destination_ip, 4);
  memcpy(&dip, b.destination_ip, 4);
  if (mask != 32 && (sip >> mask) != (dip >> mask)) return 0;
  if (b.source_port < a.source_port[0] || b.source_port > a.source_port[1])
    return 0;
  if (b.destination_port < a.destination_port[0] ||
      b.destination_port > a.destination_port[1])
    return 0;
  return 1;
}

void setmaskHash() {
  for (int i = 0; i < 33; ++i) {
    int j = 0;
    for (; j < i / 8; ++j) PT::maskHash[i][j] = 8;
    if (j < 4) {
      PT::maskHash[i][j] = i % 8;
      for (++j; j < 4; ++j) PT::maskHash[i][j] = 0;
    }
  }
}

uint64_t reverse_byte(uint64_t x) {
  uint8_t arry[8];
  uint8_t re_arry[8];
  memcpy(arry, &x, 8);
  int k = 4;
  int i = 0;
  for (; i < 4; ++i) re_arry[i] = arry[--k];
  k = 8;
  for (; i < 8; ++i) re_arry[i] = arry[--k];
  memcpy(&x, re_arry, 8);
  return x;
}
/// === ///

/// ======= ///

CacuInfo::CacuInfo(vector<PT_Rule>& _rules) {
  min_cost = 0xFFFFFFFF;
  best_fields_id = 0;
  for (auto& _r : _rules) {
    if (_r.source_mask < 4 && _r.destination_mask < 4) continue;
    PT_CacuRule* _cr = new PT_CacuRule();
    _cr->pri = _r.pri;
    _cr->mask.i_32.smask = maskBit[_r.source_mask];
    _cr->mask.i_32.dmask = maskBit[_r.destination_mask];
    _cr->mask.i_64 = reverse_byte(_cr->mask.i_64);
    memcpy(&_cr->ip.i_64, _r.source_ip, 8);
    _cr->ip.i_64 = reverse_byte(_cr->ip.i_64);
    _cr->ip.i_64 = _cr->ip.i_64 & _cr->mask.i_64;
    cRules.emplace_back(_cr);
  }
  cRules[0]->is_first = true;
  cRules[0]->size = cRules.size();
}

void CacuInfo::reset_cRules() {
  for (auto& _cr : cRules) {
    _cr->total_fetch_byte = {0};
    _cr->total_mask = {0};
    _cr->is_first = false;
    //_cr->size = 1;
    _cr->acc_inner = 1;
    _cr->acc_leaf = 0;
    _cr->acc_rule = 0;
  }
  cRules[0]->is_first = true;
  cRules[0]->size = cRules.size();
}

void CacuInfo::read_fields() {
  vector<uint8_t> tmp_fields;
  tmp_fields.resize(3);
  FILE* fp_l3 = fopen("./include/PT/L3.txt", "r");
  if (fp_l3 == nullptr) {
    fprintf(stderr, "error - can not open L3.txt\n");
    exit(0);
  }
  while (fscanf(fp_l3, "%hhu %hhu %hhu \n", &tmp_fields[0], &tmp_fields[1],
                &tmp_fields[2]) != EOF) {
    fields.emplace_back(tmp_fields);
  }
  fclose(fp_l3);
}

vector<uint8_t> CacuInfo::cacu_best_fields() {
  for (int i = 0; i < fields.size(); ++i) {
    // for (auto& x : fields[i]) cout << x << " ";
    double cur_cost = cacu_cost(fields[i]);
    if (min_cost > cur_cost) {
      min_cost = cur_cost;
      best_fields_id = i;
    }
    reset_cRules();
  }
  return fields[best_fields_id];
}

double CacuInfo::cacu_cost(vector<uint8_t>& _fields) {
  int layers = _fields.size();
  uint32_t total_inner = 0;
  double total_leaf_score = 0;
  switch (layers) {
    case 3: {
      for (int i = 0; i < 3; ++i) {
        int _field = _fields[i];
        // cacu value
        for (auto& _cr : cRules) {
          if (_cr->mask.i_8.mask[_field] == 0xFF) {
            _cr->cur_mask = 0xFF;
            _cr->cur_byte = _cr->ip.i_8.ip[_field];
            _cr->total_mask.i_8.mask[_field] = 0xFF;
            _cr->total_fetch_byte.i_8.ip[_field] = _cr->cur_byte;
          } else {
            _cr->cur_mask = 0;
            _cr->cur_byte = 0;
          }
        }
        // partition rule
        if (i < 2) {
          for (int j = 0; j < cRules.size();) {
            int _end = j + cRules[j]->size;
            total_inner += cacu_in_node(j, _end);
            j = _end;
          }
        } else {
          for (int j = 0; j < cRules.size();) {
            int _end = j + cRules[j]->size;
            total_leaf_score += cacu_in_leaf(j, _end);
            j = _end;
          }
        }
      }
      break;
    }
    case 4: {
      for (int i = 0; i < 4; ++i) {
        int _field = _fields[i];
        // cacu value
        for (auto& _cr : cRules) {
          _cr->cur_mask = _cr->mask.i_8.mask[_field];
          _cr->cur_byte = _cr->ip.i_8.ip[_field];
          _cr->total_mask.i_8.mask[_field] = _cr->cur_mask;
          _cr->total_fetch_byte.i_8.ip[_field] = _cr->cur_byte;
        }
        // partition rule
        if (i < 3) {
          for (int j = 0; j < cRules.size();) {
            int _end = j + cRules[j]->size;
            total_inner += cacu_in_node(j, _end);
            j = _end;
          }
        } else {
          for (int j = 0; j < cRules.size();) {
            int _end = j + cRules[j]->size;
            total_leaf_score += cacu_in_leaf(j, _end);
            j = _end;
          }
        }
      }
      break;
    }
    default:
      cout << "layers setting is error, should be 3 or 4." << "\n";
      exit(0);
      break;
  }
  uint32_t inverse_n = 0;
  for (int i = 0; i < cRules.size();) {
    for (int j = 0; j < i;) {
      if (cRules[j]->pri > cRules[i]->pri &&
          cRules[j]->total_fetch_byte.i_64 ==
              (cRules[i]->total_fetch_byte.i_64 & cRules[j]->total_mask.i_64)) {
        ++inverse_n;
        j += cRules[j]->tSize;
      } else {
        j += cRules[j]->size;
      }
    }
    for (int j = 0; j < 100; ++j)
      if (i < cRules.size()) i = i + cRules[i]->size;
  }
  if (total_inner > 11264) total_inner *= 20;
  // else if (layers > 3)total_inner *= 10;
  if (inverse_n > 100)
    inverse_n *= 1000;
  else if (layers == 3)
    inverse_n *= 350;
  double total_score = total_leaf_score * 0.1 + total_inner + inverse_n;
  return total_score;
}

uint32_t CacuInfo::cacu_in_node(int _start, int _end) {
  uint32_t num = 0;
  sort(cRules.begin() + _start, cRules.begin() + _end,
       [](PT_CacuRule* a, PT_CacuRule* b) -> bool {
         if (a->cur_mask != b->cur_mask)
           return a->cur_mask > b->cur_mask;
         else if (a->cur_byte != b->cur_byte)
           return a->cur_byte < b->cur_byte;
         else
           return a->pri < b->pri;
       });
  cRules[_end - 1]->size = 1;
  cRules[_end - 1]->tSize = 1;
  for (int i = _end - 2; i >= _start; --i) {
    if (cRules[i]->cur_mask == cRules[i + 1]->cur_mask) {
      cRules[i]->tSize = cRules[i + 1]->tSize + 1;
      if (cRules[i]->cur_byte == cRules[i + 1]->cur_byte) {
        cRules[i + 1]->is_first = false;
        cRules[i]->size = cRules[i + 1]->size + 1;
      } else {
        cRules[i + 1]->is_first = true;
        cRules[i]->size = 1;
        ++num;
      }
    } else {
      cRules[i + 1]->is_first = true;
      cRules[i]->size = 1;
      cRules[i]->tSize = 1;
      ++num;
    }
  }
  cRules[_start]->is_first = true;
  if (cRules[_start]->size != 1) ++num;
  return num;
}

double CacuInfo::cacu_in_leaf(int _start, int _end) {
  double score = 0;
  sort(cRules.begin() + _start, cRules.begin() + _end,
       [](PT_CacuRule* a, PT_CacuRule* b) -> bool {
         if (a->cur_mask != b->cur_mask)
           return a->cur_mask > b->cur_mask;
         else if (a->cur_byte != b->cur_byte)
           return a->cur_byte < b->cur_byte;
         else
           return a->pri < b->pri;
       });
  cRules[_end - 1]->size = 1;
  cRules[_end - 1]->tSize = 1;
  for (int i = _end - 2; i >= _start; --i) {
    if (cRules[i]->cur_mask == cRules[i + 1]->cur_mask) {
      cRules[i]->tSize = cRules[i + 1]->tSize + 1;
      if (cRules[i]->cur_byte == cRules[i + 1]->cur_byte) {
        cRules[i + 1]->is_first = false;
        cRules[i]->size = cRules[i + 1]->size + 1;
      } else {
        cRules[i + 1]->is_first = true;
        cRules[i]->size = 1;
        score += cacu_score(cRules[i + 1]->size);
      }
    } else {
      cRules[i + 1]->is_first = true;
      cRules[i]->size = 1;
      cRules[i]->tSize = 1;
      score += cacu_score(cRules[i + 1]->size);
    }
  }
  cRules[_start]->is_first = true;
  if (cRules[_start]->size != 1) score += cacu_score(cRules[_start]->size);
  return score;
}

double CacuInfo::cacu_score(uint32_t x) {
  // acl1 acl2 acl3 acl4 acl5 fw2 fw3 ipc1 ipc2
  if (x == 1)
    return 0.01;
  else if (x < 8)
    return 0.1;
  else if (x < 16)
    return 0.2;
  else if (x < 32)
    return 0.4;
  else if (x < 64)
    return 4;
  else if (x < 128)
    return 8;
  else if (x < 1024)
    return (2 * x);
  else
    return (0.04 * x * x);
}

/// ======= ///

//// ==================== ////
PTtree::PTtree(vector<uint8_t>& list, int _portField)
    : layerFields(list),
      portField(_portField),
      pTree(nullptr),
      aTree(nullptr),
      totalNodes(0),
      pt_is_modify(false),
      at_is_modify(false) {}
PTtree::~PTtree() {
  if (pTree != nullptr) {
    switch (layerFields.size()) {
      case 3: {
        freeStaticNode((IpNode_static*)pTree);
        break;
      }
      default:
        freeNode((IpNode*)pTree);
        break;
    }
  }
  if (aTree != nullptr) {
    for (auto&& c : aTree->child) {
      PortNode_static* pnode = (PortNode_static*)c.second;
      for (auto&& leaf : pnode->child) {
        delete (leaf.second);
      }
      delete (pnode);
    }
    delete (aTree);
  }
}
void PTtree::freeStaticNode(IpNode_static* node) {
  if (node->childType) {
    for (auto&& c : node->child) {
      if (c.pointer != nullptr) delete ((LeafNode*)c.pointer);
    }
  } else {
    for (auto&& c : node->child) {
      if (c.pointer != nullptr) freeStaticNode((IpNode_static*)c.pointer);
    }
  }
  delete (node);
}
void PTtree::freeNode(IpNode* node) {
  if (node->childType) {
    for (auto&& t : node->tableList) {
      for (int i = 0; i < t.child.size(); ++i)
        delete ((LeafNode*)(t.child[i].second));
    }
  } else {
    for (auto&& t : node->tableList) {
      for (int i = 0; i < t.child.size(); ++i)
        freeNode((IpNode*)(t.child[i].second));
    }
  }
  delete (node);
}
void PTtree::insert(PT_Rule& r) {
  if (r.source_mask < 4 && r.destination_mask < 4) {  // inser in assit tree
    if (aTree == nullptr) {
      aTree = new ProtoNode();
      ++totalNodes;
    }
    int proto = r.protocol[1];
    int proto_idx = aTree->table[proto];
    int lport_idx, hport_idx;
    if (portField == 0) {
      lport_idx = r.source_port[0] / 2, hport_idx = r.source_port[1] / 2;
    } else {
      lport_idx = r.destination_port[0] / 2,
      hport_idx = r.destination_port[1] / 2;
    }
    if (proto_idx == -1) {
      aTree->table[proto] = aTree->child.size();
      PortNode_static* pnode = new PortNode_static(portNodeList.size());
      ++totalNodes;
      if (lport_idx == hport_idx)
        pnode->table[lport_idx] = 0;
      else
        pnode->table[32768] = 0;
      LeafNode* lnode = new LeafNode();
      ++totalNodes;
      aLeafNodeList.emplace_back(lnode);
      lnode->rule.emplace_back(r);
      pnode->child.emplace_back(pair<uint32_t, LeafNode*>(r.pri, lnode));
      aTree->child.emplace_back(pair<uint32_t, void*>(r.pri, pnode));
      portNodeList.emplace_back(pnode);
    } else {
      PortNode_static* pnode = (PortNode_static*)aTree->child[proto_idx].second;
      if (r.pri < aTree->child[proto_idx].first)
        aTree->child[proto_idx].first = r.pri;
      int c_id;
      if (lport_idx == hport_idx)
        c_id = lport_idx;
      else
        c_id = 32768;
      int le_id = pnode->table[c_id];
      if (le_id == -1) {
        pnode->table[c_id] = pnode->child.size();
        LeafNode* lnode = new LeafNode();
        ++totalNodes;
        aLeafNodeList.emplace_back(lnode);
        lnode->rule.emplace_back(r);
        pnode->child.emplace_back(pair<uint32_t, LeafNode*>(r.pri, lnode));
      } else {
        if (r.pri < pnode->child[le_id].first)
          pnode->child[le_id].first = r.pri;
        LeafNode* lnode = pnode->child[le_id].second;
        lnode->rule.emplace_back(r);
      }
    }
  } else {  // insert in PTtree
    switch (layerFields.size()) {
      case 3: {
        if (pTree == nullptr) {
          pTree = new IpNode_static(layerFields[0], 0, 0, 0);
          ipNodeList.emplace_back(pTree);
          ++totalNodes;
        }
        IpNode_static* node = (IpNode_static*)pTree;
        int layer = 0;
        unsigned int mask, ip;
        while (layer < 2) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3];
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2];
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1];
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0];
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3];
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2];
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1];
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0];
              break;
            default:
              break;
          }
          int ip_idx = mask == 8 ? ip : 256;
          if (node->child[ip_idx].pointer == nullptr) {
            IpNode_static* newchild = new IpNode_static(
                layerFields[layer + 1], 0, layer + 1, ipNodeList.size());
            node->child[ip_idx].pointer = newchild;
            node->child[ip_idx].pri = r.pri;
            ipNodeList.emplace_back(newchild);
            ++totalNodes;
            node = newchild;
          } else {
            if (r.pri < node->child[ip_idx].pri)
              node->child[ip_idx].pri = r.pri;
            node = (IpNode_static*)node->child[ip_idx].pointer;
          }
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3];
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2];
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1];
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0];
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3];
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2];
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1];
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0];
            break;
          default:
            break;
        }
        int ip_idx = mask == 8 ? ip : 256;
        if (node->child[ip_idx].pointer == nullptr) {
          LeafNode* newchild = new LeafNode();
          newchild->rule.emplace_back(r);
          node->child[ip_idx].pointer = newchild;
          node->child[ip_idx].pri = r.pri;
          pLeafNodeList.emplace_back(newchild);
          ++totalNodes;
        } else {
          if (r.pri < node->child[ip_idx].pri) node->child[ip_idx].pri = r.pri;
          LeafNode* ln = (LeafNode*)node->child[ip_idx].pointer;
          ln->rule.emplace_back(r);
        }
        break;
      }
      default: {
        if (pTree == nullptr) {
          pTree = new IpNode(layerFields[0], 0, 0, 0);
          ipNodeList.emplace_back(pTree);
          ++totalNodes;
        }
        IpNode* node = (IpNode*)pTree;
        int totalLayer = layerFields.size();
        int layer = 0;
        unsigned int mask, ip;
        while (layer < totalLayer - 1) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3] >> (8 - mask);
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2] >> (8 - mask);
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1] >> (8 - mask);
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0] >> (8 - mask);
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
              break;
            default:
              break;
          }
          if (node->tableList.empty()) {  // do not have table, create
            IpTable t(mask);
            IpNode* newchild = new IpNode(layerFields[layer + 1], 0, layer + 1,
                                          ipNodeList.size());
            t.pri = r.pri;
            t.table[ip] = 0;
            t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
            node->tableList.emplace_back(t);
            node = newchild;
            ipNodeList.emplace_back(newchild);
            ++totalNodes;
          } else {
            list<IpTable>::iterator it = node->tableList.begin();
            for (; it != node->tableList.end(); ++it) {
              if (mask == it->mask) {  // have table
                if (it->pri > r.pri) it->pri = r.pri;
                if (it->table[ip] == -1) {  // creat child
                  IpNode* newchild = new IpNode(layerFields[layer + 1], 0,
                                                layer + 1, ipNodeList.size());
                  it->table[ip] = it->child.size();
                  it->child.emplace_back(
                      pair<uint32_t, void*>(r.pri, newchild));
                  node = newchild;
                  ipNodeList.emplace_back(newchild);
                  ++totalNodes;
                  break;
                } else {
                  if (it->child[it->table[ip]].first > r.pri)
                    it->child[it->table[ip]].first = r.pri;
                  node = (IpNode*)(it->child[it->table[ip]].second);
                  break;
                }
              }
              if (mask > it->mask) {  // find the site
                break;
              }
            }
            if (it == node->tableList.end() ||
                mask != it->mask) {  // creat table
              IpTable t(mask);
              IpNode* newchild = new IpNode(layerFields[layer + 1], 0,
                                            layer + 1, ipNodeList.size());
              t.pri = r.pri;
              t.table[ip] = t.child.size();
              t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
              node->tableList.emplace(it, t);
              node = newchild;
              ipNodeList.emplace_back(newchild);
              ++totalNodes;
            }
          }
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3] >> (8 - mask);
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2] >> (8 - mask);
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1] >> (8 - mask);
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0] >> (8 - mask);
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
            break;
          default:
            break;
        }
        if (node->tableList.empty()) {  // do not have table, create
          IpTable t(mask);
          LeafNode* newchild = new LeafNode();
          newchild->rule.emplace_back(r);
          t.pri = r.pri;
          t.table[ip] = 0;
          t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
          node->tableList.emplace_back(t);
          pLeafNodeList.emplace_back(newchild);
          ++totalNodes;
        } else {
          list<IpTable>::iterator it = node->tableList.begin();
          for (; it != node->tableList.end(); ++it) {
            if (mask == it->mask) {  // have table
              if (it->pri > r.pri) it->pri = r.pri;
              if (it->table[ip] == -1) {  // creat child
                LeafNode* newchild = new LeafNode();
                newchild->rule.emplace_back(r);
                it->table[ip] = it->child.size();
                it->child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
                pLeafNodeList.emplace_back(newchild);
                ++totalNodes;
                break;
              } else {
                if (it->child[it->table[ip]].first > r.pri)
                  it->child[it->table[ip]].first = r.pri;
                ((LeafNode*)(it->child[it->table[ip]].second))
                    ->rule.emplace_back(r);
                break;
              }
            }
            if (mask > it->mask) {  // find the site
              break;
            }
          }
          if (it == node->tableList.end() || mask != it->mask) {  // creat table
            IpTable t(mask);
            LeafNode* newchild = new LeafNode();
            newchild->rule.emplace_back(r);
            t.pri = r.pri;
            t.table[ip] = t.child.size();
            t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
            node->tableList.emplace(it, t);
            pLeafNodeList.emplace_back(newchild);
            ++totalNodes;
          }
        }
        break;
      }
    }
  }
}

void PTtree::insert_up(PT_Rule& r) {
  if (r.source_mask < 4 && r.destination_mask < 4) {  // inser in assit tree
    if (aTree == nullptr) {
      aTree = new ProtoNode();
      ++totalNodes;
    }
    int proto = r.protocol[1];
    int proto_idx = aTree->table[proto];
    int lport_idx, hport_idx;
    if (portField == 0) {
      lport_idx = r.source_port[0] / 2, hport_idx = r.source_port[1] / 2;
    } else {
      lport_idx = r.destination_port[0] / 2,
      hport_idx = r.destination_port[1] / 2;
    }
    if (proto_idx == -1) {
      aTree->table[proto] = aTree->child.size();
      PortNode_static* pnode = new PortNode_static(portNodeList.size());
      ++totalNodes;
      if (lport_idx == hport_idx)
        pnode->table[lport_idx] = 0;
      else
        pnode->table[32768] = 0;
      LeafNode* lnode = new LeafNode();
      ++totalNodes;
      aLeafNodeList.emplace_back(lnode);
      lnode->rule.emplace_back(r);
      pnode->child.emplace_back(pair<uint32_t, LeafNode*>(r.pri, lnode));
      aTree->child.emplace_back(pair<uint32_t, void*>(r.pri, pnode));
      portNodeList.emplace_back(pnode);
    } else {
      PortNode_static* pnode = (PortNode_static*)aTree->child[proto_idx].second;
      if (r.pri < aTree->child[proto_idx].first)
        aTree->child[proto_idx].first = r.pri;
      int c_id;
      if (lport_idx == hport_idx)
        c_id = lport_idx;
      else
        c_id = 32768;
      int le_id = pnode->table[c_id];
      if (le_id == -1) {
        pnode->table[c_id] = pnode->child.size();
        LeafNode* lnode = new LeafNode();
        ++totalNodes;
        aLeafNodeList.emplace_back(lnode);
        lnode->rule.emplace_back(r);
        pnode->child.emplace_back(pair<uint32_t, LeafNode*>(r.pri, lnode));
      } else {
        if (r.pri < pnode->child[le_id].first)
          pnode->child[le_id].first = r.pri;
        LeafNode* lnode = pnode->child[le_id].second;

        int k = 0;
        for (; k < lnode->rule.size(); ++k)
          if (r.pri < lnode->rule[k].pri) break;
        lnode->rule.emplace(lnode->rule.begin() + k, r);
      }
    }
  } else {  // insert in PTtree
    switch (layerFields.size()) {
      case 3: {
        if (pTree == nullptr) {
          pTree = new IpNode_static(layerFields[0], 0, 0, 0);
          ipNodeList.emplace_back(pTree);
          ++totalNodes;
        }
        IpNode_static* node = (IpNode_static*)pTree;
        int layer = 0;
        unsigned int mask, ip;
        while (layer < 2) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3];
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2];
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1];
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0];
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3];
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2];
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1];
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0];
              break;
            default:
              break;
          }
          int ip_idx = mask == 8 ? ip : 256;
          if (node->child[ip_idx].pointer == nullptr) {
            IpNode_static* newchild = new IpNode_static(
                layerFields[layer + 1], 0, layer + 1, ipNodeList.size());
            node->child[ip_idx].pointer = newchild;
            node->child[ip_idx].pri = r.pri;
            ipNodeList.emplace_back(newchild);
            ++totalNodes;
            node = newchild;
          } else {
            if (r.pri < node->child[ip_idx].pri)
              node->child[ip_idx].pri = r.pri;
            node = (IpNode_static*)node->child[ip_idx].pointer;
          }
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3];
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2];
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1];
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0];
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3];
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2];
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1];
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0];
            break;
          default:
            break;
        }
        int ip_idx = mask == 8 ? ip : 256;
        if (node->child[ip_idx].pointer == nullptr) {
          LeafNode* newchild = new LeafNode();
          newchild->rule.emplace_back(r);
          node->child[ip_idx].pointer = newchild;
          node->child[ip_idx].pri = r.pri;
          pLeafNodeList.emplace_back(newchild);
          ++totalNodes;
        } else {
          if (r.pri < node->child[ip_idx].pri) node->child[ip_idx].pri = r.pri;
          LeafNode* ln = (LeafNode*)node->child[ip_idx].pointer;
          int k = 0;
          for (; k < ln->rule.size(); ++k)
            if (r.pri < ln->rule[k].pri) break;
          ln->rule.emplace(ln->rule.begin() + k, r);
        }
        break;
      }
      default: {
        if (pTree == nullptr) {
          pTree = new IpNode(layerFields[0], 0, 0, 0);
          ipNodeList.emplace_back(pTree);
          ++totalNodes;
        }
        IpNode* node = (IpNode*)pTree;
        int totalLayer = layerFields.size();
        int layer = 0;
        unsigned int mask, ip;
        while (layer < totalLayer - 1) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3] >> (8 - mask);
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2] >> (8 - mask);
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1] >> (8 - mask);
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0] >> (8 - mask);
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
              break;
            default:
              break;
          }
          if (node->tableList.empty()) {  // do not have table, create
            IpTable t(mask);
            IpNode* newchild = new IpNode(layerFields[layer + 1], 0, layer + 1,
                                          ipNodeList.size());
            t.pri = r.pri;
            t.table[ip] = 0;
            t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
            node->tableList.emplace_back(t);
            node = newchild;
            ipNodeList.emplace_back(newchild);
            ++totalNodes;
          } else {
            list<IpTable>::iterator it = node->tableList.begin();
            for (; it != node->tableList.end(); ++it) {
              if (mask == it->mask) {  // have table
                if (it->pri > r.pri) it->pri = r.pri;
                if (it->table[ip] == -1) {  // creat child
                  IpNode* newchild = new IpNode(layerFields[layer + 1], 0,
                                                layer + 1, ipNodeList.size());
                  it->table[ip] = it->child.size();
                  it->child.emplace_back(
                      pair<uint32_t, void*>(r.pri, newchild));
                  node = newchild;
                  ipNodeList.emplace_back(newchild);
                  ++totalNodes;
                  break;
                } else {
                  if (it->child[it->table[ip]].first > r.pri)
                    it->child[it->table[ip]].first = r.pri;
                  node = (IpNode*)(it->child[it->table[ip]].second);
                  break;
                }
              }
              if (mask > it->mask) {  // find the site
                break;
              }
            }
            if (it == node->tableList.end() ||
                mask != it->mask) {  // creat table
              IpTable t(mask);
              IpNode* newchild = new IpNode(layerFields[layer + 1], 0,
                                            layer + 1, ipNodeList.size());
              t.pri = r.pri;
              t.table[ip] = t.child.size();
              t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
              node->tableList.emplace(it, t);
              node = newchild;
              ipNodeList.emplace_back(newchild);
              ++totalNodes;
            }
          }
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3] >> (8 - mask);
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2] >> (8 - mask);
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1] >> (8 - mask);
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0] >> (8 - mask);
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
            break;
          default:
            break;
        }
        if (node->tableList.empty()) {  // do not have table, create
          IpTable t(mask);
          LeafNode* newchild = new LeafNode();
          newchild->rule.emplace_back(r);
          t.pri = r.pri;
          t.table[ip] = 0;
          t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
          node->tableList.emplace_back(t);
          pLeafNodeList.emplace_back(newchild);
          ++totalNodes;
        } else {
          list<IpTable>::iterator it = node->tableList.begin();
          for (; it != node->tableList.end(); ++it) {
            if (mask == it->mask) {  // have table
              if (it->pri > r.pri) it->pri = r.pri;
              if (it->table[ip] == -1) {  // creat child
                LeafNode* newchild = new LeafNode();
                newchild->rule.emplace_back(r);
                it->table[ip] = it->child.size();
                it->child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
                pLeafNodeList.emplace_back(newchild);
                ++totalNodes;
                break;
              } else {
                if (it->child[it->table[ip]].first > r.pri)
                  it->child[it->table[ip]].first = r.pri;
                LeafNode* ln = (LeafNode*)it->child[it->table[ip]].second;
                int k = 0;
                for (; k < ln->rule.size(); ++k)
                  if (r.pri < ln->rule[k].pri) break;
                ln->rule.emplace(ln->rule.begin() + k, r);
                break;
              }
            }
            if (mask > it->mask) {  // find the site
              break;
            }
          }
          if (it == node->tableList.end() || mask != it->mask) {  // creat table
            IpTable t(mask);
            LeafNode* newchild = new LeafNode();
            newchild->rule.emplace_back(r);
            t.pri = r.pri;
            t.table[ip] = t.child.size();
            t.child.emplace_back(pair<uint32_t, void*>(r.pri, newchild));
            node->tableList.emplace(it, t);
            pLeafNodeList.emplace_back(newchild);
            ++totalNodes;
          }
        }
        break;
      }
    }
  }
}
bool PTtree::remove(PT_Rule& r) {
  if (r.source_mask < 4 && r.destination_mask < 4) {  // remove in assit tree
    if (aTree == nullptr) {
      return false;
    }
    int proto = r.protocol[1];
    int proto_idx = aTree->table[proto];
    int lport_idx, hport_idx;
    if (portField == 0) {
      lport_idx = r.source_port[0] / 2, hport_idx = r.source_port[1] / 2;
    } else {
      lport_idx = r.destination_port[0] / 2,
      hport_idx = r.destination_port[1] / 2;
    }
    if (proto_idx == -1) {
      return false;
    } else {
      PortNode_static* p_node =
          (PortNode_static*)aTree->child[proto_idx].second;
      int c_id;
      if (lport_idx == hport_idx)
        c_id = lport_idx;
      else
        c_id = 32768;
      int le_id = p_node->table[c_id];
      if (le_id == -1) {
        return false;
      } else {
        LeafNode* lnode = p_node->child[le_id].second;
        for (int i = 0; i < lnode->rule.size(); ++i) {
          if (lnode->rule[i].pri == r.pri) {
            lnode->rule.erase(lnode->rule.begin() + i);
            return true;
          }
        }
        return false;
      }
    }
  } else {  // remove in PTtree
    if (pTree == nullptr) {
      return false;
    }
    switch (layerFields.size()) {
      case 3: {
        IpNode_static* node = (IpNode_static*)pTree;
        int layer = 0;
        unsigned int mask, ip;
        while (layer < 2) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3];
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2];
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1];
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0];
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3];
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2];
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1];
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0];
              break;
            default:
              break;
          }
          int ip_idx = mask == 8 ? ip : 256;
          if (node->child[ip_idx].pointer == nullptr)
            return false;
          else
            node = (IpNode_static*)node->child[ip_idx].pointer;
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3];
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2];
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1];
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0];
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3];
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2];
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1];
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0];
            break;
          default:
            break;
        }
        int ip_idx = mask == 8 ? ip : 256;
        if (node->child[ip_idx].pointer == nullptr)
          return false;
        else {
          LeafNode* ln = (LeafNode*)node->child[ip_idx].pointer;
          for (int i = 0; i < ln->rule.size(); ++i) {
            if (ln->rule[i].pri == r.pri) {
              ln->rule.erase(ln->rule.begin() + i);
              return true;
            }
          }
          return false;
        }
      }
      default: {
        if (pTree == nullptr) {
          return false;
        }
        IpNode* node = (IpNode*)pTree;
        int totalLayer = layerFields.size();
        int layer = 0;
        unsigned int mask, ip;
        while (layer < totalLayer - 1) {
          switch (node->field) {
            case 0:
              mask = maskHash[(unsigned int)r.source_mask][0];
              ip = (unsigned int)r.source_ip[3] >> (8 - mask);
              break;
            case 1:
              mask = maskHash[(unsigned int)r.source_mask][1];
              ip = (unsigned int)r.source_ip[2] >> (8 - mask);
              break;
            case 2:
              mask = maskHash[(unsigned int)r.source_mask][2];
              ip = (unsigned int)r.source_ip[1] >> (8 - mask);
              break;
            case 3:
              mask = maskHash[(unsigned int)r.source_mask][3];
              ip = (unsigned int)r.source_ip[0] >> (8 - mask);
              break;
            case 4:
              mask = maskHash[(unsigned int)r.destination_mask][0];
              ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
              break;
            case 5:
              mask = maskHash[(unsigned int)r.destination_mask][1];
              ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
              break;
            case 6:
              mask = maskHash[(unsigned int)r.destination_mask][2];
              ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
              break;
            case 7:
              mask = maskHash[(unsigned int)r.destination_mask][3];
              ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
              break;
            default:
              break;
          }
          list<IpTable>::iterator it = node->tableList.begin();
          for (; it != node->tableList.end(); ++it) {
            if (mask == it->mask) {       // have table
              if (it->table[ip] == -1) {  // no child
                return false;
              } else {
                node = (IpNode*)(it->child[it->table[ip]].second);
                break;
              }
            }
            if (mask > it->mask) {  // find the site
              break;
            }
          }
          if (it == node->tableList.end() || mask != it->mask) {  // no table
            return false;
          }
          ++layer;
        }
        // process leafnode
        node->childType = 1;
        switch (node->field) {
          case 0:
            mask = maskHash[(unsigned int)r.source_mask][0];
            ip = (unsigned int)r.source_ip[3] >> (8 - mask);
            break;
          case 1:
            mask = maskHash[(unsigned int)r.source_mask][1];
            ip = (unsigned int)r.source_ip[2] >> (8 - mask);
            break;
          case 2:
            mask = maskHash[(unsigned int)r.source_mask][2];
            ip = (unsigned int)r.source_ip[1] >> (8 - mask);
            break;
          case 3:
            mask = maskHash[(unsigned int)r.source_mask][3];
            ip = (unsigned int)r.source_ip[0] >> (8 - mask);
            break;
          case 4:
            mask = maskHash[(unsigned int)r.destination_mask][0];
            ip = (unsigned int)r.destination_ip[3] >> (8 - mask);
            break;
          case 5:
            mask = maskHash[(unsigned int)r.destination_mask][1];
            ip = (unsigned int)r.destination_ip[2] >> (8 - mask);
            break;
          case 6:
            mask = maskHash[(unsigned int)r.destination_mask][2];
            ip = (unsigned int)r.destination_ip[1] >> (8 - mask);
            break;
          case 7:
            mask = maskHash[(unsigned int)r.destination_mask][3];
            ip = (unsigned int)r.destination_ip[0] >> (8 - mask);
            break;
          default:
            break;
        }
        list<IpTable>::iterator it = node->tableList.begin();
        for (; it != node->tableList.end(); ++it) {
          if (mask == it->mask) {       // have table
            if (it->table[ip] == -1) {  // no child
              return false;
            } else {
              LeafNode* ln = ((LeafNode*)(it->child[it->table[ip]].second));
              for (int i = 0; i < ln->rule.size(); ++i) {
                if (ln->rule[i].pri == r.pri) {
                  ln->rule.erase(ln->rule.begin() + i);
                  return true;
                }
              }
              break;
            }
          }
          if (mask > it->mask) {  // find the site
            break;
          }
        }
        return false;
      }
    }
  }
}

int PTtree::search(const PT_Packet& p) {
  unsigned int pSip, pDip;
  unsigned char pProto;
  /* JIA  unsigned short */ uint16_t pSport, pDport;
  pProto = p.protocol;
  memcpy(&pSip, p.source_ip, 4);
  memcpy(&pDip, p.destination_ip, 4);
  pSport = p.source_port;
  pDport = p.destination_port;

  unsigned int mip[4];
  for (int i = 0; i < layerFields.size(); ++i) {
    switch (layerFields[i]) {
      case 0:
        mip[i] = (unsigned int)p.source_ip[3];
        break;
      case 1:
        mip[i] = (unsigned int)p.source_ip[2];
        break;
      case 2:
        mip[i] = (unsigned int)p.source_ip[1];
        break;
      case 3:
        mip[i] = (unsigned int)p.source_ip[0];
        break;
      case 4:
        mip[i] = (unsigned int)p.destination_ip[3];
        break;
      case 5:
        mip[i] = (unsigned int)p.destination_ip[2];
        break;
      case 6:
        mip[i] = (unsigned int)p.destination_ip[1];
        break;
      case 7:
        mip[i] = (unsigned int)p.destination_ip[0];
        break;
      default:
        break;
    }
  }
  unsigned int res = 0xFFFFFFFF;

  // search in pTree
  switch (layerFields.size()) {
    case 3: {
      IpNode_static* node_1 = (IpNode_static*)pTree;
      unsigned int i_1[2] = {mip[0], 256};
      for (int i = 0; i < 2; ++i) {
        if (node_1->child[i_1[i]].pointer == nullptr ||
            node_1->child[i_1[i]].pri > res)
          continue;
        IpNode_static* node_2 = (IpNode_static*)node_1->child[i_1[i]].pointer;
        unsigned int i_2[2] = {mip[1], 256};
        for (int j = 0; j < 2; ++j) {
          if (node_2->child[i_2[j]].pointer == nullptr ||
              node_2->child[i_2[j]].pri > res)
            continue;
          IpNode_static* node_3 = (IpNode_static*)node_2->child[i_2[j]].pointer;
          unsigned int i_3[2] = {mip[2], 256};
          for (int k = 0; k < 2; ++k) {
            if (node_3->child[i_3[k]].pointer == nullptr ||
                node_3->child[i_3[k]].pri > res)
              continue;
            LeafNode* ln = (LeafNode*)node_3->child[i_3[k]].pointer;
            for (auto&& r : ln->rule) {
              if (res < r.pri) break;
              if (pProto != r.protocol[1] && r.protocol[0] != 0)
                continue;  // check protocol
              if (pDport < r.destination_port[0] ||
                  r.destination_port[1] < pDport)
                continue;  // if destination port not match, check next
              if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                continue;  // if source port not match, check next
              unsigned int m_bit =
                  32 -
                  (unsigned int)
                      r.destination_mask;  // comput the bit number need to move
              unsigned int _ip;
              if (m_bit != 32) {
                memcpy(&_ip, r.destination_ip, 4);
                if (pDip >> m_bit != _ip >> m_bit)
                  continue;  // if destination ip not match, check next
              }
              m_bit =
                  32 - (unsigned int)
                           r.source_mask;  // comput the bit number need to move
              if (m_bit != 32) {
                memcpy(&_ip, r.source_ip, 4);
                if (pSip >> m_bit != _ip >> m_bit)
                  continue;  // if source ip not match, check next
              }
              res = r.pri;
              break;
            }
          }
        }
      }
      break;
    }
    case 4: {
      IpNode* node_1 = (IpNode*)pTree;
      unsigned int ip_idx = 0;
      for (list<IpTable>::iterator it_1 = node_1->tableList.begin();
           it_1 != node_1->tableList.end(); ++it_1) {
        ip_idx = mip[0] >> (8 - it_1->mask);
        if (it_1->pri > res || it_1->table[ip_idx] == -1 ||
            it_1->child[it_1->table[ip_idx]].first > res)
          continue;
        IpNode* node_2 = (IpNode*)(it_1->child[it_1->table[ip_idx]].second);
        for (list<IpTable>::iterator it_2 = node_2->tableList.begin();
             it_2 != node_2->tableList.end(); ++it_2) {
          ip_idx = mip[1] >> (8 - it_2->mask);
          if (it_2->pri > res || it_2->table[ip_idx] == -1 ||
              it_2->child[it_2->table[ip_idx]].first > res)
            continue;
          IpNode* node_3 = (IpNode*)(it_2->child[it_2->table[ip_idx]].second);
          for (list<IpTable>::iterator it_3 = node_3->tableList.begin();
               it_3 != node_3->tableList.end(); ++it_3) {
            ip_idx = mip[2] >> (8 - it_3->mask);
            if (it_3->pri > res || it_3->table[ip_idx] == -1 ||
                it_3->child[it_3->table[ip_idx]].first > res)
              continue;
            IpNode* node_4 = (IpNode*)(it_3->child[it_3->table[ip_idx]].second);
            for (list<IpTable>::iterator it_4 = node_4->tableList.begin();
                 it_4 != node_4->tableList.end(); ++it_4) {
              ip_idx = mip[3] >> (8 - it_4->mask);
              if (it_4->pri > res || it_4->table[ip_idx] == -1 ||
                  it_4->child[it_4->table[ip_idx]].first > res)
                continue;
              LeafNode* ln =
                  (LeafNode*)(it_4->child[it_4->table[ip_idx]].second);
              for (auto&& r : ln->rule) {
                if (res < r.pri) break;
                if (pProto != r.protocol[1] && r.protocol[0] != 0)
                  continue;  // check protocol
                if (pDport < r.destination_port[0] ||
                    r.destination_port[1] < pDport)
                  continue;  // if destination port not match, check next
                if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                  continue;  // if source port not match, check next
                unsigned int m_bit =
                    32 -
                    (unsigned int)r.destination_mask;  // comput the bit number
                                                       // need to move
                unsigned int _ip;
                if (m_bit != 32) {
                  memcpy(&_ip, r.destination_ip, 4);
                  if (pDip >> m_bit != _ip >> m_bit)
                    continue;  // if destination ip not match, check next
                }
                m_bit =
                    32 -
                    (unsigned int)
                        r.source_mask;  // comput the bit number need to move
                if (m_bit != 32) {
                  memcpy(&_ip, r.source_ip, 4);
                  if (pSip >> m_bit != _ip >> m_bit)
                    continue;  // if source ip not match, check next
                }
                res = r.pri;
                break;
              }
            }
          }
        }
      }
      break;
    }
    default:
      break;
  }

  if (aTree != nullptr) {
    int proto_idx[2] = {aTree->table[pProto], aTree->table[0]};
    int port_idx[2];
    for (int i = 0; i < 2; ++i) {
      if (proto_idx[i] != -1 && res > aTree->child[proto_idx[i]].first) {
        PortNode_static* pnode =
            (PortNode_static*)aTree->child[proto_idx[i]].second;
        if (portField == 0)
          port_idx[0] = pnode->table[pSport / 2];
        else
          port_idx[0] = pnode->table[pDport / 2];
        port_idx[1] = pnode->table[32768];
        for (int j = 0; j < 2; ++j) {
          if (port_idx[j] != -1 && res > pnode->child[port_idx[j]].first) {
            LeafNode* ln = pnode->child[port_idx[j]].second;
            for (auto&& r : ln->rule) {
              if (res < r.pri) break;
              if (pDport < r.destination_port[0] ||
                  r.destination_port[1] < pDport)
                continue;  // if destination port not match, check next
              if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                continue;  // if source port not match, check next
              unsigned int m_bit =
                  32 -
                  (unsigned int)
                      r.destination_mask;  // comput the bit number need to move
              unsigned int _ip;
              if (m_bit != 32) {
                memcpy(&_ip, r.destination_ip, 4);
                if (pDip >> m_bit != _ip >> m_bit)
                  continue;  // if destination ip not match, check next
              }
              m_bit =
                  32 - (unsigned int)
                           r.source_mask;  // comput the bit number need to move
              if (m_bit != 32) {
                memcpy(&_ip, r.source_ip, 4);
                if (pSip >> m_bit != _ip >> m_bit)
                  continue;  // if source ip not match, check next
              }
              res = r.pri;
              break;
            }
          }
        }
      }
    }
  }
  return res;
}
int PTtree::search_with_log(PT_Packet& p, ACL_LOG& log) {
  unsigned int pSip, pDip;
  unsigned char pProto;
  /* JIA  unsigned short */ uint16_t pSport, pDport;
  pProto = p.protocol;
  memcpy(&pSip, p.source_ip, 4);
  memcpy(&pDip, p.destination_ip, 4);
  pSport = p.source_port;
  pDport = p.destination_port;

  unsigned int mip[4];
  for (int i = 0; i < layerFields.size(); ++i) {
    switch (layerFields[i]) {
      case 0:
        mip[i] = (unsigned int)p.source_ip[3];
        break;
      case 1:
        mip[i] = (unsigned int)p.source_ip[2];
        break;
      case 2:
        mip[i] = (unsigned int)p.source_ip[1];
        break;
      case 3:
        mip[i] = (unsigned int)p.source_ip[0];
        break;
      case 4:
        mip[i] = (unsigned int)p.destination_ip[3];
        break;
      case 5:
        mip[i] = (unsigned int)p.destination_ip[2];
        break;
      case 6:
        mip[i] = (unsigned int)p.destination_ip[1];
        break;
      case 7:
        mip[i] = (unsigned int)p.destination_ip[0];
        break;
      default:
        break;
    }
  }
  unsigned int res = 0xFFFFFFFF;

  // search in pTree
  if (pTree != nullptr) {
    switch (layerFields.size()) {
      case 3: {
        IpNode_static* node_1 = (IpNode_static*)pTree;
        unsigned int i_1[2] = {mip[0], 256};
        log.ipNodeList.emplace_back(node_1);
        for (int i = 0; i < 2; ++i) {
          if (node_1->child[i_1[i]].pointer == nullptr ||
              node_1->child[i_1[i]].pri > res)
            continue;
          IpNode_static* node_2 = (IpNode_static*)node_1->child[i_1[i]].pointer;
          unsigned int i_2[2] = {mip[1], 256};
          log.ipNodeList.emplace_back(node_2);
          for (int j = 0; j < 2; ++j) {
            if (node_2->child[i_2[j]].pointer == nullptr ||
                node_2->child[i_2[j]].pri > res)
              continue;
            IpNode_static* node_3 =
                (IpNode_static*)node_2->child[i_2[j]].pointer;
            unsigned int i_3[2] = {mip[2], 256};
            log.ipNodeList.emplace_back(node_3);
            for (int k = 0; k < 2; ++k) {
              if (node_3->child[i_3[k]].pointer == nullptr ||
                  node_3->child[i_3[k]].pri > res)
                continue;
              LeafNode* ln = (LeafNode*)node_3->child[i_3[k]].pointer;
              log.pLeafNodeList.emplace_back(ln);
              for (auto&& r : ln->rule) {
                ++log.rules;
                if (res < r.pri) break;
                if (pProto != r.protocol[1] && r.protocol[0] != 0)
                  continue;  // check protocol
                if (pDport < r.destination_port[0] ||
                    r.destination_port[1] < pDport)
                  continue;  // if destination port not match, check next
                if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                  continue;  // if source port not match, check next
                unsigned int m_bit =
                    32 -
                    (unsigned int)r.destination_mask;  // comput the bit number
                                                       // need to move
                unsigned int _ip;
                if (m_bit != 32) {
                  memcpy(&_ip, r.destination_ip, 4);
                  if (pDip >> m_bit != _ip >> m_bit)
                    continue;  // if destination ip not match, check next
                }
                m_bit =
                    32 -
                    (unsigned int)
                        r.source_mask;  // comput the bit number need to move
                if (m_bit != 32) {
                  memcpy(&_ip, r.source_ip, 4);
                  if (pSip >> m_bit != _ip >> m_bit)
                    continue;  // if source ip not match, check next
                }
                res = r.pri;
                break;
              }
            }
          }
        }
        log.tables += log.ipNodeList.size();
        break;
      }
      case 4: {
        IpNode* node_1 = (IpNode*)pTree;
        unsigned int ip_idx = 0;
        log.ipNodeList.emplace_back(node_1);
        for (list<IpTable>::iterator it_1 = node_1->tableList.begin();
             it_1 != node_1->tableList.end(); ++it_1) {
          ip_idx = mip[0] >> (8 - it_1->mask);
          if (it_1->pri > res) continue;
          ++log.tables;
          if (it_1->table[ip_idx] == -1 ||
              it_1->child[it_1->table[ip_idx]].first > res)
            continue;
          IpNode* node_2 = (IpNode*)(it_1->child[it_1->table[ip_idx]].second);
          log.ipNodeList.emplace_back(node_2);
          for (list<IpTable>::iterator it_2 = node_2->tableList.begin();
               it_2 != node_2->tableList.end(); ++it_2) {
            ip_idx = mip[1] >> (8 - it_2->mask);
            if (it_2->pri > res) continue;
            ++log.tables;
            if (it_2->table[ip_idx] == -1 ||
                it_2->child[it_2->table[ip_idx]].first > res)
              continue;
            IpNode* node_3 = (IpNode*)(it_2->child[it_2->table[ip_idx]].second);
            log.ipNodeList.emplace_back(node_3);
            for (list<IpTable>::iterator it_3 = node_3->tableList.begin();
                 it_3 != node_3->tableList.end(); ++it_3) {
              ip_idx = mip[2] >> (8 - it_3->mask);
              if (it_3->pri > res) continue;
              ++log.tables;
              if (it_3->table[ip_idx] == -1 ||
                  it_3->child[it_3->table[ip_idx]].first > res)
                continue;
              IpNode* node_4 =
                  (IpNode*)(it_3->child[it_3->table[ip_idx]].second);
              log.ipNodeList.emplace_back(node_4);
              for (list<IpTable>::iterator it_4 = node_4->tableList.begin();
                   it_4 != node_4->tableList.end(); ++it_4) {
                ip_idx = mip[3] >> (8 - it_4->mask);
                if (it_4->pri > res) continue;
                ++log.tables;
                if (it_4->table[ip_idx] == -1 ||
                    it_4->child[it_4->table[ip_idx]].first > res)
                  continue;
                LeafNode* ln =
                    (LeafNode*)(it_4->child[it_4->table[ip_idx]].second);
                log.pLeafNodeList.emplace_back(ln);
                for (auto&& r : ln->rule) {
                  ++log.rules;
                  if (res < r.pri) break;
                  if (pProto != r.protocol[1] && r.protocol[0] != 0)
                    continue;  // check protocol
                  if (pDport < r.destination_port[0] ||
                      r.destination_port[1] < pDport)
                    continue;  // if destination port not match, check next
                  if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                    continue;  // if source port not match, check next
                  unsigned int m_bit =
                      32 -
                      (unsigned int)r.destination_mask;  // comput the bit
                                                         // number need to move
                  unsigned int _ip;
                  if (m_bit != 32) {
                    memcpy(&_ip, r.destination_ip, 4);
                    if (pDip >> m_bit != _ip >> m_bit)
                      continue;  // if destination ip not match, check next
                  }
                  m_bit =
                      32 -
                      (unsigned int)
                          r.source_mask;  // comput the bit number need to move
                  if (m_bit != 32) {
                    memcpy(&_ip, r.source_ip, 4);
                    if (pSip >> m_bit != _ip >> m_bit)
                      continue;  // if source ip not match, check next
                  }
                  res = r.pri;
                  break;
                }
              }
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }

  if (aTree != nullptr) {
    int proto_idx[2] = {aTree->table[pProto], aTree->table[0]};
    int port_idx[2];
    for (int i = 0; i < 2; ++i) {
      if (proto_idx[i] != -1 && res > aTree->child[proto_idx[i]].first) {
        PortNode_static* pnode =
            (PortNode_static*)aTree->child[proto_idx[i]].second;
        if (portField == 0)
          port_idx[0] = pnode->table[pSport / 2];
        else
          port_idx[0] = pnode->table[pDport / 2];
        port_idx[1] = pnode->table[32768];
        log.portNodeList.emplace_back(pnode);
        for (int j = 0; j < 2; ++j) {
          if (port_idx[j] != -1 && res > pnode->child[port_idx[j]].first) {
            LeafNode* ln = pnode->child[port_idx[j]].second;
            log.aLeafNodeList.emplace_back(ln);
            for (auto&& r : ln->rule) {
              ++log.rules;
              if (res < r.pri) break;
              if (pDport < r.destination_port[0] ||
                  r.destination_port[1] < pDport)
                continue;  // if destination port not match, check next
              if (pSport < r.source_port[0] || r.source_port[1] < pSport)
                continue;  // if source port not match, check next
              unsigned int m_bit =
                  32 -
                  (unsigned int)
                      r.destination_mask;  // comput the bit number need to move
              unsigned int _ip;
              if (m_bit != 32) {
                memcpy(&_ip, r.destination_ip, 4);
                if (pDip >> m_bit != _ip >> m_bit)
                  continue;  // if destination ip not match, check next
              }
              m_bit =
                  32 - (unsigned int)
                           r.source_mask;  // comput the bit number need to move
              if (m_bit != 32) {
                memcpy(&_ip, r.source_ip, 4);
                if (pSip >> m_bit != _ip >> m_bit)
                  continue;  // if source ip not match, check next
              }
              res = r.pri;
              break;
            }
          }
        }
      }
    }
  }

  log.tables += log.portNodeList.size() + 1;
  log.innerNodes = log.ipNodeList.size() + log.portNodeList.size() + 1;
  log.leafNodes = log.pLeafNodeList.size() + log.aLeafNodeList.size();

  return res;
}

bool PTtree::update(vector<PT_Rule>& rules, int num, Timer& timer) {
  const size_t ruleNum = rules.size();
  vector<PT_Rule> newRule;
  vector<int> rd_idx;
  random_device seed;
  mt19937 rd(seed());
  uniform_int_distribution<> dis(0, ruleNum * 0.7);
  for (int i = 0; i < num; ++i) {
    int cur_idx = dis(rd);
    while (find(rd_idx.begin(), rd_idx.end(), cur_idx) != rd_idx.end())
      cur_idx = dis(rd);
    PT_Rule r = rules[cur_idx];
    newRule.emplace_back(r);
    rd_idx.emplace_back(cur_idx);
  }

  unsigned long long delete_time = 0, _delete_time = 0;
  unsigned long long insert_time = 0, _insert_time = 0;
  // remove
  for (int i = 0; i < num; ++i) {
    timer.timeReset();
    bool res = this->remove(newRule[i]);
    _delete_time = timer.elapsed_ns();
    delete_time += _delete_time;
    if (!res) {
      fprintf(stderr, "error-can not find rule! Remove rules failed!");
      return res;
    }
  }
  // insert
  for (int i = 0; i < num; ++i) {
    timer.timeReset();
    this->insert_up(newRule[i]);
    _insert_time = timer.elapsed_ns();
    insert_time += _insert_time;
  }

  cout << "|- Average delete time: " << delete_time / (num) << "ns\n";
  cout << "|- Average insert time: " << insert_time / (num) << "ns\n";
  cout << "|- Average update time: "
       << (delete_time + insert_time) / (num * 2.0) << "ns\n";
  return true;
}

void PTtree::print_node_info(const size_t rules) {
  std::cout << "|- Total nodes num:          " << this->totalNodes << "\n";
  std::cout << "|- Total inner nodes num:    "
            << this->ipNodeList.size() + this->portNodeList.size() + 1 << "\n";
  std::cout << "|- Total leaf nodes num:     "
            << this->pLeafNodeList.size() + this->aLeafNodeList.size() << "\n";
  std::cout << "|- IpNode num:               " << this->ipNodeList.size()
            << "\n";
  std::cout << "|- pTree leaf nodes num:     " << this->pLeafNodeList.size()
            << "\n";
  std::cout << "|- ProtoNode num:            " << this->portNodeList.size()
            << "\n";
  std::cout << "|- aTree leaf nodes num:     " << this->aLeafNodeList.size()
            << "\n";
  std::cout << "|- Average leaf node size:   "
            << (double)rules / (double)(this->pLeafNodeList.size() +
                                        this->aLeafNodeList.size())
            << "\n";
  double equ_1 = 0;
  double rang_1to100 = 0;
  double lager_100 = 0;
  int max_leaf = 0;
  for (auto leaf : this->pLeafNodeList) {
    if (leaf->rule.size() > max_leaf) max_leaf = leaf->rule.size();
    if (leaf->rule.size() == 1)
      ++equ_1;
    else if (leaf->rule.size() < 100)
      ++rang_1to100;
    else
      ++lager_100;
  }
  std::cout << "|- Leaf node size->1:        " << equ_1 << "\n";
  std::cout << "|- Leaf node size->(1, 100]: " << rang_1to100 << "\n";
  std::cout << "|- Leaf node size->(100, +]: " << lager_100 << "\n";
  std::cout << "|- Max leaf node size:       " << max_leaf << "\n";

  FILE* fp = nullptr;
  std::cout
      << "|- Write pTree inner node infomation to pInnerNode_info.txt...\n";
  fp = fopen("./INFO/PT_pInnerNode_info.txt", "w");
  fprintf(fp, "IpNode [ID LAYER FIELD TABLE_NUM CHILD_NUM]\n\n");
  switch (this->layerFields.size()) {
    case 3: {
      for (auto& node : this->ipNodeList) {
        IpNode_static* n = (IpNode_static*)node;
        int c_num = 0;
        for (int i = 0; i < 257; ++i)
          if (n->child[i].pointer != nullptr) ++c_num;
        fprintf(fp, "%u\t%u\t%u\t1\t%d\n", n->id, n->layer, n->field, c_num);
      }
      break;
    }
    default: {
      for (auto& node : this->ipNodeList) {
        IpNode* n = (IpNode*)node;
        int c_num = 0;
        list<IpTable>::iterator it = n->tableList.begin();
        for (; it != n->tableList.end(); ++it) c_num += it->child.size();
        fprintf(fp, "%u\t%u\t%u\t%lu\t%d\n", n->id, n->layer, n->field,
                n->tableList.size(), c_num);
      }
      break;
    }
  }
  fclose(fp);

  std::cout << "|- Write pTree leaf node infomation to pLeafNode_info.txt...\n";
  fp = fopen("./INFO/PT_pLeafNode_info.txt", "w");
  fprintf(fp,
          "Leaf Node [ID SIG] (SIG={[1, 1], (1, 16], (16, 32], (32, 64], (64, "
          "128],  (128, +)})\n|- Rule [PRI SIP DIP SPORT DPORT PROTOCOL]\n");
  for (int i = 0; i < this->pLeafNodeList.size(); ++i) {
    int psize = this->pLeafNodeList[i]->rule.size();
    if (psize == 1)
      fprintf(fp, "\n%d\t%d [1, 1]\n", i, psize);
    else if (psize < 16)
      fprintf(fp, "\n%d\t%d (1, 16]\n", i, psize);
    else if (psize < 32)
      fprintf(fp, "\n%d\t%d (16, 32]\n", i, psize);
    else if (psize < 64)
      fprintf(fp, "\n%d\t%d (32, 64]\n", i, psize);
    else if (psize < 128)
      fprintf(fp, "\n%d\t%d (64, 128]\n", i, psize);
    else
      fprintf(fp, "\n%d\t%d (128, +)\n", i, psize);
    for (auto r : this->pLeafNodeList[i]->rule)
      fprintf(fp,
              "|- "
              "%u\t%u.%u.%u.%u/%u\t\t%u.%u.%u.%u/%u\t\t%u:%u\t\t%u:%u\t\t%u\n",
              r.pri, r.source_ip[3], r.source_ip[2], r.source_ip[1],
              r.source_ip[0], r.source_mask, r.destination_ip[3],
              r.destination_ip[2], r.destination_ip[1], r.destination_ip[0],
              r.destination_mask, r.source_port[0], r.source_port[1],
              r.destination_port[0], r.destination_port[1], r.protocol[1]);
  }
  fclose(fp);

  std::cout
      << "|- Write aTree inner node infomation to aInnerNode_info.txt...\n";
  fp = fopen("./INFO/PT_aInnerNode_info.txt", "w");
  fprintf(fp, "Protocol Node [ID TABLE_NUM CHILD_NUM]\n\n");
  fprintf(fp, "0\t1\t%lu\n\n", this->aTree->child.size());
  fprintf(fp, "Port Node [ID TABLE_NUM CHILD_NUM]\n\n");
  for (auto node : this->portNodeList) {
    PortNode_static* n = (PortNode_static*)node;
    fprintf(fp, "%u\t2\t%lu\n", n->id, n->child.size());
  }
  fclose(fp);

  std::cout << "|- Write aTree leaf node infomation to aLeafNode_info.txt...\n";
  fp = fopen("./INFO/PT_aLeafNode_info.txt", "w");
  fprintf(fp,
          "Leaf Node [ID SIZE]\n|- Rule [PRI SIP DIP SPORT DPORT PROTOCOL]\n");
  for (int i = 0; i < this->aLeafNodeList.size(); ++i) {
    fprintf(fp, "\n%d\t%lu\n", i, this->aLeafNodeList[i]->rule.size());
    for (auto r : this->aLeafNodeList[i]->rule)
      fprintf(fp,
              "|- "
              "%u\t%u.%u.%u.%u/%u\t\t%u.%u.%u.%u/%u\t\t%u:%u\t\t%u:%u\t\t%u\n",
              r.pri, r.source_ip[3], r.source_ip[2], r.source_ip[1],
              r.source_ip[0], r.source_mask, r.destination_ip[3],
              r.destination_ip[2], r.destination_ip[1], r.destination_ip[0],
              r.destination_mask, r.source_port[0], r.source_port[1],
              r.destination_port[0], r.destination_port[1], r.protocol[1]);
  }
  fclose(fp);
}

size_t PTtree::get_ipNode_mem(IpNode* node) {
  size_t sum = sizeof(IpNode);
  for (list<IpTable>::iterator it = node->tableList.begin();
       it != node->tableList.end(); ++it) {
    sum = sum + sizeof(IpTable) + sizeof(short) * it->table.size() +
          sizeof(pair<uint32_t, void*>) * it->child.size();
  }
  return sum;
}

size_t PTtree::get_leafNode_mem(LeafNode* node) {
  size_t sum = sizeof(LeafNode) + sizeof(PT_Rule) * node->rule.size();
  return sum;
}

size_t PTtree::get_static_mem(IpNode_static* node) {
  size_t sum = sizeof(IpNode_static);
  if (node->childType == 0) {
    for (int i = 0; i < 257; ++i) {
      if (node->child[i].pointer != nullptr)
        sum += get_static_mem((IpNode_static*)node->child[i].pointer);
    }
  } else {
    for (int i = 0; i < 257; ++i) {
      if (node->child[i].pointer != nullptr)
        sum += get_leafNode_mem((LeafNode*)node->child[i].pointer);
    }
  }
  return sum;
}

size_t PTtree::get_mem(IpNode* node) {
  size_t sum = get_ipNode_mem(node);
  if (node->childType == 0) {
    for (list<IpTable>::iterator it = node->tableList.begin();
         it != node->tableList.end(); ++it) {
      for (int i = 0; i < it->child.size(); ++i) {
        sum += get_mem((IpNode*)(it->child[i].second));
      }
    }
  } else {
    for (list<IpTable>::iterator it = node->tableList.begin();
         it != node->tableList.end(); ++it) {
      for (int i = 0; i < it->child.size(); ++i) {
        sum += get_leafNode_mem((LeafNode*)(it->child[i].second));
      }
    }
  }
  return sum;
}

size_t PTtree::mem() {
  size_t sum = 0;
  if (aTree != nullptr) {
    sum += sizeof(ProtoNode) + sizeof(short) * aTree->table.size() +
           sizeof(pair<uint32_t, PortNode_static*>) * aTree->child.size();
    for (auto&& c : aTree->child) {
      PortNode_static* pnode = (PortNode_static*)c.second;
      sum += sizeof(PortNode_static) +
             sizeof(pair<uint32_t, LeafNode*>) * pnode->child.size();
      for (auto&& leaf : pnode->child) {
        sum += sizeof(LeafNode) + leaf.second->rule.size() * sizeof(PT_Rule);
      }
    }
  }
  switch (layerFields.size()) {
    case 3: {
      return sum + get_static_mem((IpNode_static*)pTree);
    }
    default:
      return sum + get_mem((IpNode*)pTree);
  }
}
//// ==================== ////
}  // namespace PT
