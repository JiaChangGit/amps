#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
將五個 *_prediction_3_result.txt 轉成 Excel，
並找出在至少「兩個 tail」同時出現的 packet_ID。
"""

import os
import re
from collections import Counter
from typing import Dict, Iterable, Set, Any

import pandas as pd

# ------------------------------------------------------------
# 讀檔設定
# ------------------------------------------------------------
TXT_FILES = [
    "PT_prediction_3_result.txt",
    "DBT_prediction_3_result.txt",
    "KSet_prediction_3_result.txt",
    "DT_prediction_3_result.txt",
    "MT_prediction_3_result.txt",
]

PATTERN = re.compile(r"Packet (\d+)\s+RealTime\(ns\) ([\d.]+)")
OUTPUT_XLSX = "AVG_D_Results_BySheet.xlsx"

# ------------------------------------------------------------
# 小工具：找出至少出現在 N 個集合的元素
# ------------------------------------------------------------
def find_common_ids(
    tail_dict: Dict[str, pd.DataFrame],
    *,
    key: str = "packet_ID",
    min_occurrence: int = 2,
    labels: Iterable[str] = ("PT", "DBT", "KSet", "DT", "MT"),
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

with pd.ExcelWriter(OUTPUT_XLSX) as writer:
    for txt_path in TXT_FILES:
        with open(txt_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        # 抽取 (packet_ID, TimeR)
        records = [
            (int(m.group(1)), float(m.group(2)))
            for line in lines
            if (m := PATTERN.search(line))
        ]
        df = pd.DataFrame(records, columns=["packet_ID", "TimeR (ns)"])

        # 依 AVG 設門檻（2×AVG）
        avg_time = df["TimeR (ns)"].mean()
        threshold = 2 * avg_time
        print(f"[{txt_path}] AVG = {avg_time:.2f} ns, Threshold = {threshold:.2f} ns")

        # 篩 tail
        df_tail = df[df["TimeR (ns)"] >= threshold].copy()

        # 標記來源
        base_name = os.path.basename(txt_path).replace("_prediction_3_result.txt", "")
        df_tail["Source"] = base_name

        # 寫入 Excel（未排序與已排序）
        df_tail.to_excel(writer, index=False, sheet_name=f"{base_name}_Unsorted")
        df_tail.sort_values("TimeR (ns)").reset_index(drop=True).to_excel(
            writer, index=False, sheet_name=f"{base_name}_Sorted"
        )

        # 存到字典以便後續交集運算
        tail_dict[base_name] = df_tail

    # --------------------------------------------------------
    # 找出「至少落在 2 個 tail」的 packet_ID
    # --------------------------------------------------------
    common_ids = find_common_ids(tail_dict, min_occurrence=2)

    print(f"✅ 至少出現在 2 個 tail 的 packet_ID 數量：{len(common_ids)}")
    if common_ids:
        preview = sorted(common_ids)[:20]
        print("前 20 個 packet_ID：", preview)

    # 寫入 Excel Sheet
    pd.DataFrame(sorted(common_ids), columns=["Common packet_ID"]).to_excel(
        writer, index=False, sheet_name="Common_in_Tail"
    )

print(f"📄 全部結果已寫入：{OUTPUT_XLSX}")
