#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
將五個 *_prediction_3_result.txt 轉成 Excel，
並找出在至少「兩個 tail」同時出現的 packet_ID。
以「五個 txt 各自的 95 百分位數中之最小值」作為 tail threshold。
"""

import os
import re
from collections import Counter
from typing import Dict, Iterable, Set, Any

import pandas as pd
import numpy as np

# ------------------------------------------------------------
# 讀檔設定
# ------------------------------------------------------------
TXT_FILES = [
    "PT_prediction_3_result.txt",
    "DBT_prediction_3_result.txt",
    #"KSet_prediction_3_result.txt",
    "DT_prediction_3_result.txt",
    #"MT_prediction_3_result.txt",
]

PATTERN = re.compile(r"Packet (\d+)\s+RealTime\(ns\) ([\d.]+)")
OUTPUT_XLSX = "AVG_Results_BySheet.xlsx"

# ------------------------------------------------------------
# 小工具：找出至少出現在 N 個集合的元素
# ------------------------------------------------------------
def find_common_ids(
    tail_dict: Dict[str, pd.DataFrame],
    *,
    key: str = "packet_ID",
    min_occurrence: int = 2,
    #labels: Iterable[str] = ("PT", "DBT", "KSet", "DT", "MT"),
    labels: Iterable[str] = ("PT", "DBT", "DT"),
) -> Set[Any]:
    """
    回傳「至少同時出現在 min_occurrence 個 DataFrame[key]」的元素集合。
    """
    counter: Counter[Any] = Counter()
    for label in labels:
        ids = set(tail_dict.get(label, pd.DataFrame()).get(key, []))
        counter.update(ids)
    return {pid for pid, cnt in counter.items() if cnt >= min_occurrence}


# ------------------------------------------------------------
# 主流程：解析 txt → DataFrame → Excel
# ------------------------------------------------------------
tail_dict: Dict[str, pd.DataFrame] = {}
percentile_95_list = []

df_all: Dict[str, pd.DataFrame] = {}

# 先收集 95-th 百分位數
for txt_path in TXT_FILES:
    with open(txt_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    records = [
        (int(m.group(1)), float(m.group(2)))
        for line in lines
        if (m := PATTERN.search(line))
    ]
    df = pd.DataFrame(records, columns=["packet_ID", "TimeR (ns)"])

    p95 = np.percentile(df["TimeR (ns)"], 95)
    percentile_95_list.append(p95)
    base_name = os.path.basename(txt_path).replace("_prediction_3_result.txt", "")
    df_all[base_name] = df

# 最小的 95 百分位數作為全域 threshold
global_threshold = min(percentile_95_list)
print(f"🔍 Global threshold (min 95-th percentile) = {global_threshold:.2f} ns")

# 寫入 Excel 並標記 tail
with pd.ExcelWriter(OUTPUT_XLSX) as writer:
    for label, df in df_all.items():
        df_tail = df[df["TimeR (ns)"] > global_threshold].copy()
        df_tail["Source"] = label
        tail_dict[label] = df_tail

        df_tail.to_excel(writer, index=False, sheet_name=f"{label}_Unsorted")
        df_tail.sort_values("TimeR (ns)").reset_index(drop=True).to_excel(
            writer, index=False, sheet_name=f"{label}_Sorted"
        )

    # 找出共同 tail packet_ID
    common_ids = find_common_ids(tail_dict, min_occurrence=2)
    print(f"✅ 至少出現在 2 個 tail 的 packet_ID 數量：{len(common_ids)}")
    if common_ids:
        preview = sorted(common_ids)[:20]
        print("前 20 個 packet_ID：", preview)

    pd.DataFrame(sorted(common_ids), columns=["Common packet_ID"]).to_excel(
        writer, index=False, sheet_name="Common_in_Tail"
    )

print(f"📄 全部結果已寫入：{OUTPUT_XLSX}")
