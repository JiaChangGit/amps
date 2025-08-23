#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_all_metrics_to_csv.py

遞迴掃描當前目錄及子目錄中所有 .txt 檔，
一次擷取以下 13 個指標並輸出到 result.csv：
  - indivi_<X>_y Mean
  - indivi_<X>_y 99th Percentile
    (X ∈ {PT, DBT, KSet, DT, MT})
  - AVG predict time(Model-3  Omp)
  - AVG search with predict time(Model-3 + Omp)
  - Model-3_y 99th Percentile
"""

import os
import re
import csv

# 定義要擷取的所有指標及對應的正則
METRIC_REGEX = {
    # 原本 10 個指標
    'PT_mean':        re.compile(r'indivi_PT_y\s+Mean\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DBT_mean':       re.compile(r'indivi_DBT_y\s+Mean\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'KSet_mean':      re.compile(r'indivi_KSet_y\s+Mean\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DT_mean':        re.compile(r'indivi_DT_y\s+Mean\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'MT_mean':        re.compile(r'indivi_MT_y\s+Mean\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'PT_p99':         re.compile(r'indivi_PT_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DBT_p99':        re.compile(r'indivi_DBT_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'KSet_p99':       re.compile(r'indivi_KSet_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DT_p99':         re.compile(r'indivi_DT_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'MT_p99':         re.compile(r'indivi_MT_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'PT_y_stddev':    re.compile(r'indivi_PT_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DBT_y_stddev':    re.compile(r'indivi_DBT_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'KSet_y_stddev':    re.compile(r'indivi_KSet_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'DT_y_stddev':    re.compile(r'indivi_DT_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'MT_y_stddev':    re.compile(r'indivi_MT_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),

    'M3_sig_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-3\s+Single\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M3_omp_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-3\s+Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M3_omp_search':  re.compile(r'AVG\s+search\s+with\s+predict\s+time\s*\(\s*Model-3\s*\+\s*Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M3_y_p99':       re.compile(r'Model-3_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'M3_y_stddev':    re.compile(r'Model-3_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    
    'M5_sig_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-5\s+Single\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M5_omp_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-5\s+Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M5_omp_search':  re.compile(r'AVG\s+search\s+with\s+predict\s+time\s*\(\s*Model-5\s*\+\s*Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M5_y_p99':       re.compile(r'Model-5_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'M5_y_stddev':    re.compile(r'Model-5_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    
    'M11_sig_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-11\s+Single\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M11_omp_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*Model-11\s+Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M11_omp_search':  re.compile(r'AVG\s+search\s+with\s+predict\s+time\s*\(\s*Model-11\s*\+\s*Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'M11_y_p99':       re.compile(r'Model-11_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'M11_y_stddev':    re.compile(r'Model-11_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    
    'KNN_sig_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*KNN\s+Single\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'KNN_omp_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*KNN\s+Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'KNN_omp_search':  re.compile(r'AVG\s+search\s+with\s+predict\s+time\s*\(\s*KNN\s*\+\s*Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'KNN_y_p99':       re.compile(r'KNN_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'KNN_y_stddev':    re.compile(r'KNN_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    
    'BF_sig_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*BloomFilter\s+Single\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'BF_omp_pred':    re.compile(r'AVG\s+predict\s+time\s*\(\s*BloomFilter\s+Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'BF_omp_search':  re.compile(r'AVG\s+search\s+time\s+with\s+predict\s*\(\s*BloomFilter\s*\+\s*Omp\s*\)\s*:\s*([0-9]*\.?[0-9]+)\s*ns', re.IGNORECASE),
    'BF_y_p99':       re.compile(r'Total_y\s+99th\s+Percentile\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
    'BF_y_stddev':    re.compile(r'Total_y\s+StdDev\s*:\s*([0-9]*\.?[0-9]+)', re.IGNORECASE),
}

OUTPUT_CSV = 'result.csv'

def extract_metrics(file_path):
    """
    從單個檔案中擷取所有指標值，回傳 dict：
    若某指標找不到，則值為 None。
    """
    results = {key: None for key in METRIC_REGEX}
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            for key, regex in METRIC_REGEX.items():
                if results[key] is None:
                    m = regex.search(line)
                    if m:
                        results[key] = float(m.group(1))
            # 若已經擷取完全部指標，可提前跳出
            if all(v is not None for v in results.values()):
                break
    return results

def extract_dataset_name(filename):
    """從 main_c_xxx.txt 擷取出 xxx 作為 dataset 名稱；不符則回 None。"""
    m = re.match(r'^main_c_(.+)\.txt$', filename, re.IGNORECASE)
    return m.group(1) if m else None

def main():
    fieldnames = ['dataset'] + list(METRIC_REGEX.keys())
    rows = []

    for root, _, files in os.walk('.'):
        for fn in files:
            if fn.lower().endswith('.txt'):
                dataset = extract_dataset_name(fn)
                if not dataset:
                    continue
                fullpath = os.path.join(root, fn)
                metrics = extract_metrics(fullpath)
                # 至少有一項指標非 None 才納入
                if any(v is not None for v in metrics.values()):
                    row = {'dataset': dataset}
                    row.update(metrics)
                    rows.append(row)

    if not rows:
        print("⚠️ 未擷取到任何指標值。")
        return

    # 寫入 CSV
    with open(OUTPUT_CSV, 'w', newline='', encoding='utf-8') as csvf:
        writer = csv.DictWriter(csvf, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    print(f"✅ 共寫入 {len(rows)} 筆資料到 {OUTPUT_CSV}")

if __name__ == '__main__':
    main()
