import re
import pandas as pd

# 檔案名稱
input_file = "Total_prediction_3_result.txt"
output_excel = "Cache_search_analyz.xlsx"

# 要提取的欄位
fields = ["PT", "DBT", "KSet", "DT", "MT", "min", "MAX"]
results = []

# 正規表達式（匹配欄位和值，無冒號，以空格分隔）
pattern = re.compile(r"\b(PT|DBT|KSet|DT|MT|min|MAX)\s+([-+]?[0-9]*\.?[0-9]+)\b")

# 解析檔案
with open(input_file, "r", encoding="utf-8") as f:
    for line in f:
        current = {key: None for key in fields}
        matches = pattern.findall(line)
        
        for key, val in matches:
            if key in fields:
                current[key] = float(val)
        
        # 若所有欄位皆有值，則記錄
        if all(current[k] is not None for k in fields):
            results.append(current)

# 匯出至 Excel
df = pd.DataFrame(results)
df.to_excel(output_excel, index=False)
print(f"✅ 匯出完成：{output_excel}")
