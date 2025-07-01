# 匯入必要的 Python 庫
import pandas as pd        # 用於讀取和處理 Excel 檔案
import numpy as np         # 用於數值計算，例如生成累積概率
import matplotlib.pyplot as plt  # 用於繪製圖表

# 定義 Excel 檔案的路徑
file_path = "Cache_search_analyz.xlsx"  # 檔案名稱，假設與程式碼位於同一目錄

# 讀取 Excel 檔案中的 "Sheet1" 工作表
# 使用 pandas 的 read_excel 函數讀取指定檔案和工作表
df = pd.read_excel(file_path, sheet_name="Sheet1")

# 提取 "Time (ns)" 列的資料
# 從 DataFrame 中選取 "Time (ns)" 列，並存入 PT_time_data 變數
# PT	DBT	KSet	DT	MT
# PT_time_data = df["PT"]
# PT_time_data = df["DBT"]
# PT_time_data = df["KSet"]
# PT_time_data = df["DT"]
PT_time_data = df["MT"]

# 對時間資料進行排序
# 使用 sort_values 方法將 PT_time_data 從小到大排序，以便計算 CDF
PT_sorted_time = PT_time_data.sort_values()

# 計算資料的總數量
# 使用 len 函數計算 PT_sorted_time 中的資料點數量
n = len(PT_sorted_time)

# 計算累積分佈函數 (CDF) 的 Y 軸值（累積概率）
# np.arange(1, n + 1) 創建一個從 1 到 n 的數組，除以 n 得到每個資料點的累積概率
cdf_y = np.arange(1, n + 1) / n

# 找到累積概率 >= 0.9 的資料索引
# 使用 np.where 找出 cdf_y 中大於等於 0.9 的位置，以便限制 Y 軸範圍
index = np.where(cdf_y >= 0.9)[0]

# 提取對應的時間值和累積概率
# 根據 index 從 PT_sorted_time 和 cdf_y 中提取對應的部分資料
x = PT_sorted_time.iloc[index]  # X 軸：時間 (ns)
y = cdf_y[index]             # Y 軸：累積概率

# 繪製 CDF 圖
# 使用 matplotlib 的 plot 函數繪製散點圖，marker='.' 表示用點表示資料，linestyle='none' 表示不連接線條
plt.plot(x, y, marker='.', linestyle='none')

# plt.xlabel("PT Search time (ns)")      # 設置 X 軸標籤為 "時間 (ns)"
# plt.xlabel("DBT Search time (ns)")
# plt.xlabel("KSet Search time (ns)")
# plt.xlabel("DT Search time (ns)")
plt.xlabel("MT Search time (ns)")

plt.ylabel("CDF")  # 設置 Y 軸標籤為 "累積分佈函數 (CDF)"

# plt.title("PT Search time (ns) with CDF")  # 設置圖表標題
# plt.title("DBT Search time (ns) with CDF")
# plt.title("KSet Search time (ns) with CDF")
# plt.title("DT Search time (ns) with CDF")
plt.title("MT Search time (ns) with CDF")

plt.ylim(0.9, 1.0)          # 限制 Y 軸範圍為 0.9 到 1.0
plt.grid(True)              # 加入網格線，提升圖表可讀性
plt.show()                  # 顯示繪製完成的圖表
