import re
import pandas as pd

# 讀取 TXT 檔案
#file_path = "./DBT_IndivResults"
#file_path = "./BloomResults"
#file_path = "./Total_knn_result"
file_path = "./Total_model_3_result"

tail_name = ".txt"
with open(file_path + tail_name, "r", encoding="utf-8") as file:
    lines = file.readlines()

# 新的正則表達式：擷取 Packet 編號 與 Time(ns)
pattern = re.compile(r"Packet\s+(\d+)\s+Time\(ns\)\s+([\d.]+)")

# 存儲提取的數據
data = []
for line in lines:
    match = pattern.search(line)
    if match:
        # packet_id = int(match.group(1))
        time_ns = float(match.group(2))
        # data.append((packet_id, time_ns))
        data.append((time_ns))

# 轉換為 DataFrame
# df = pd.DataFrame(data, columns=["Packet", "Time (ns)"])
df = pd.DataFrame(data, columns=["Time (ns)"])

# 儲存成 Excel
excel_path = file_path + ".xlsx"
df.to_excel(excel_path, index=False)

print(f"數據已成功提取並儲存至 {excel_path}")
