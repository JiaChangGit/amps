#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_memory.py:

遞迴掃描當前資料夾及所有子資料夾中的 main_c_*.txt，
擷取：
  1. KSet 區段中，每 4 筆 "Total memory: xxx(KB)" → 加總後轉為 MB（每檔可能多組）
  2. 所有 "Memory footprint: xxxMB"（每檔兩筆，若少補 None）
  3. 所有 "Total memory xxx MB"（每檔兩筆，若少補 None）
  4. 所有 "Total(MB): xxx"（每檔三筆，若少補 None）
  5. KD-tree index memory (MB): xxx（擷取第一筆，若無則為 None）
  6. Bloom index memory (MB): xxx（擷取第一筆，若無則為 None）

從檔名 main_c_acl2_5k.txt 提取 acl2_5k 作為 dataset，
最後輸出到 Excel 檔 extracted_metrics.xlsx。
"""

import os
import re
import pandas as pd

# ---------- 編譯所有需要的正則 ----------
PAT_KSET = re.compile(r"Total memory:\s*([0-9]*\.?[0-9]+)\s*\(KB\)", re.IGNORECASE)
PAT_FOOT = re.compile(r"Memory footprint:\s*([0-9]*\.?[0-9]+)\s*MB", re.IGNORECASE)
PAT_TMB  = re.compile(r"Total memory\s*([0-9]*\.?[0-9]+)\s*MB", re.IGNORECASE)
PAT_TP   = re.compile(r"Total\(MB\):\s*([0-9]*\.?[0-9]+)", re.IGNORECASE)

# 對應你貼出的格式（可能有前導的 |*** 等），擷取 KD-tree 與 Bloom index memory
PAT_KDTREE = re.compile(r"(?:\|?\*+)?\s*KD-?tree\s+index\s+memory\s*\(MB\)\s*:\s*([0-9]*\.?[0-9]+)", re.IGNORECASE)
PAT_BLOOM  = re.compile(r"(?:\|?\*+)?\s*Bloom\s+index\s+memory\s*\(MB\)\s*:\s*([0-9]*\.?[0-9]+)", re.IGNORECASE)

def extract_from_text(txt):
    """
    從純文字中擷取四種指標與 index memory，並回傳 kset_mb_list, mf_list, tmb_list, tp_list, kd_val, bloom_val
    其中：
      - kset_mb_list: 每 4 筆 KB 為一組，轉為 MB 的 list
      - mf_list: Memory footprint → 至少長度 2（不足以 None 補齊）
      - tmb_list: Total memory xxx MB → 至少長度 2
      - tp_list: Total(MB): xxx → 至少長度 3
      - kd_val: KD-tree index memory (MB) 第一筆或 None
      - bloom_val: Bloom index memory (MB) 第一筆或 None
    """
    # 1) KSet 區段： 每 4 筆 KB → 轉 MB；每檔可能多組
    kb_vals = [float(x) for x in PAT_KSET.findall(txt)]
    kset_mb = []
    for i in range(0, len(kb_vals), 4):
        group = kb_vals[i:i+4]
        if len(group) == 4:
            kset_mb.append(sum(group) / 1024.0)  # KB -> MB

    # 2) Memory footprint → 最少補到 2 筆
    mf = [float(x) for x in PAT_FOOT.findall(txt)]
    while len(mf) < 2:
        mf.append(None)

    # 3) Total memory xxx MB → 最少補到 2 筆
    tmb = [float(x) for x in PAT_TMB.findall(txt)]
    while len(tmb) < 2:
        tmb.append(None)

    # 4) Total(MB): xxx → 最少補到 3 筆
    tp = [float(x) for x in PAT_TP.findall(txt)]
    while len(tp) < 3:
        tp.append(None)

    # 5) KD-tree index memory (MB) → 取第一筆（若有）
    kd_vals = PAT_KDTREE.findall(txt)
    kd_val = float(kd_vals[0]) if kd_vals else None

    # 6) Bloom index memory (MB) → 取第一筆（若有）
    bloom_vals = PAT_BLOOM.findall(txt)
    bloom_val = float(bloom_vals[0]) if bloom_vals else None

    return kset_mb, mf, tmb, tp, kd_val, bloom_val

def main():
    rows = []
    # 遞迴掃描所有子目錄
    for root, _, files in os.walk('.'):
        for fn in files:
            lowfn = fn.lower()
            if lowfn.startswith("main_c_") and lowfn.endswith(".txt"):
                path = os.path.join(root, fn)
                try:
                    with open(path, encoding='utf-8', errors='ignore') as f:
                        txt = f.read()
                except Exception as e:
                    print(f"Warning: could not read file {path}: {e}")
                    continue

                kset_mb, mf, tmb, tp, kd_val, bloom_val = extract_from_text(txt)

                # dataset 由檔名 main_c_xxx.txt 取出 xxx
                dataset = fn[len("main_c_"):-4]

                # 取 kset 第一組（若存在），並做四捨五入；否則為 None
                kset_first_mb = round(kset_mb[0], 6) if (len(kset_mb) > 0 and kset_mb[0] is not None) else None

                row = {
                    "dataset": dataset,
                    "kset_total_mb": kset_first_mb,
                    "PT_RL": mf[0],
                    "PT_indiv": mf[1],
                    "DBT_RL": tmb[0],
                    "DBT_indiv": tmb[1],
                    "DT_RL": tp[0],
                    "MT": tp[1],
                    "DT_indiv": tp[2],
                    "KD_tree_index_mb": round(kd_val, 6) if kd_val is not None else None,
                    "Bloom_index_mb": round(bloom_val, 6) if bloom_val is not None else None,
                }
                rows.append(row)

    if not rows:
        print("⚠️ No main_c_*.txt files found or no data extracted.")
        return

    # 匯出到 Excel
    df = pd.DataFrame(rows)
    output_file = "extracted_metrics.xlsx"
    df.to_excel(output_file, index=False)
    print(f"Done. Processed {len(rows)} files, results written to {output_file}")

if __name__ == "__main__":
    main()
