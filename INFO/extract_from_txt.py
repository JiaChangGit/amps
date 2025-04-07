import re
import pandas as pd

# 讀取 TXT 檔案
file_path = "./INFO/PT_prediction_result"  # 替換成你的 TXT 檔案名稱
tail_name = ".txt"
with open(file_path + tail_name, "r", encoding="utf-8") as file:
    lines = file.readlines()

# 使用正則表達式提取 Result 和 Time(ns)
#pattern = re.compile(r"Result\s+(\d+)\s+Time\(ns\)\s+([\d.]+)")
pattern = re.compile(r"Packet (\d+)\s+Predict3 ([\d.]+)\s+Predict11 ([\d.]+)\s+RealTime\(ns\) ([\d.]+)")

# 存儲提取的數據
data = []
for line in lines:
    match = pattern.search(line)
#    if match:
#        result = int(match.group(1))
#        time = float(match.group(2))
#        data.append((result, time))
    if match:
        result = int(match.group(1))
        time = float(match.group(2))
        time_11 = float(match.group(3))
        time_R = float(match.group(4))
        data.append((result, time,time_11,time_R))

# 轉換為 DataFrame
#df = pd.DataFrame(data, columns=["Result", "Time (ns)"])
df = pd.DataFrame(data, columns=["Result", "Time (ns)","Time11 (ns)","TimeR (ns)"])

# 將數據儲存到 Excel
excel_path = file_path + ".xlsx"
df.to_excel(excel_path, index=False)

print(f"數據已成功提取並儲存至 {excel_path}")
