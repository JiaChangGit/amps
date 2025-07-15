# 匯入必要的 Python 套件
import pandas as pd                    # 用於讀取 Excel 檔案
import numpy as np                     # 用於數值計算與陣列操作
import matplotlib.pyplot as plt        # 用於繪圖顯示 CDF

# Excel 檔案路徑（假設與程式位於相同資料夾）
file_path = "Cache_search_analyz.xlsx"

# 讀取 Excel 中的 "Sheet1"
df = pd.read_excel(file_path, sheet_name="Sheet1")

# 定義要處理的五種分類器欄位名稱
columns = ["PT", "DBT", "KSet", "DT", "MT"]

# 為每個分類器指定顏色與圖例標籤
colors = ['blue', 'green', 'red', 'purple', 'orange']
labels = ["PT", "DBT", "KSet", "DT", "MT"]

# 建立畫布，設定圖表大小
plt.figure(figsize=(10, 6))

# 儲存各分類器中 CDF >= 0.9 對應的最大時間值，用來決定 X 軸上限
x_max_candidates = []

# 逐一處理每個分類器的搜尋時間資料
for col, color, label in zip(columns, colors, labels):
    # 取得並清洗該分類器欄位資料（去除 NaN）
    time_data = df[col].dropna()

    # 將資料從小到大排序
    sorted_time = time_data.sort_values()

    # 計算資料總數
    n = len(sorted_time)

    # 建立對應的累積機率 CDF（Y 軸）：第 i 個數據對應 i/n
    cdf_y = np.arange(1, n + 1) / n

    # 找出累積機率大於等於 0.9 的資料索引
    index = np.where(cdf_y >= 0.9)[0]

    # 取得對應的 X（時間）與 Y（累積機率）資料
    x = sorted_time.iloc[index]
    y = cdf_y[index]

    # 記錄最大時間，用於設定 X 軸範圍
    x_max_candidates.append(x.max())

    # 畫出該分類器的 CDF（點狀線）
    plt.plot(x, y, marker='.', linestyle='none', color=color, label=label, markersize = 0.4)

# 決定所有分類器中 CDF ≥ 0.9 部分的最大時間
x_upper_limit = max(x_max_candidates)

# 設定圖表標題與 X/Y 軸標籤
plt.title("CDF of Search Time for All Classifiers (Zoomed: Top 10%)")  # 圖表標題
plt.xlabel("Search time with log base=2 (ns)")        # X 軸：搜尋時間（單位為 ns）
plt.ylabel("CDF")                     # Y 軸：累積分佈機率

# 限制 Y 軸只顯示 CDF >= 0.9 的部分
plt.ylim(0.9, 1.0)

## 限制 X 軸範圍為 0 到（最大搜尋時間 * 1.05），避免右邊空白太大
#plt.xlim(0, x_upper_limit * 1.05)
plt.xlim(128, 57000)

# 對數軸
plt.xscale('log', base=2) 
# 顯示圖例（標註各分類器）
plt.legend(loc='lower right',markerscale = 20)

# 顯示網格線，幫助讀取數據位置
plt.grid(True)

# 儲存圖表為 PNG 圖片，解析度為 600 dpi
plt.savefig("search_time_cdf.png", dpi=600)

# 顯示圖表
plt.show()
