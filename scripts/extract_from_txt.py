import re
import pandas as pd
import os

# 要處理的 TXT 檔案清單
txt_files = [
    "PT_prediction_3_result.txt",
    "DBT_prediction_3_result.txt",
    "KSet_prediction_3_result.txt"
]

# 正則表達式：擷取 packet ID 與 RealTime(ns)
pattern = re.compile(r"Packet (\d+)\s+Predict ([\d.]+)\s+RealTime\(ns\) ([\d.]+)")

# 輸出 Excel 路徑
output_path = "AVG_D_Results_BySheet.xlsx"

# 儲存每個模型的 tail DataFrame
tail_dict = {}

# 建立 ExcelWriter
with pd.ExcelWriter(output_path) as writer:
    for txt_path in txt_files:
        with open(txt_path, "r", encoding="utf-8") as file:
            lines = file.readlines()

        records = []
        for line in lines:
            match = pattern.search(line)
            if match:
                packet_ID = int(match.group(1))
                time_R = float(match.group(3))
                records.append((packet_ID, time_R))

        # 轉 DataFrame
        df = pd.DataFrame(records, columns=["packet_ID", "TimeR (ns)"])

        # 計算 AVG，設定閾值 = 2 倍 AVG
        avg_time = df["TimeR (ns)"].mean()
        threshold = 2 * avg_time
        print(f"[{txt_path}] AVG: {avg_time:.2f}, Threshold: {threshold:.2f}")

        # 篩選 tail 資料
        df_tail = df[df["TimeR (ns)"] >= threshold].copy()

        # 來源標籤
        base_name = os.path.basename(txt_path).replace("_prediction_3_result.txt", "")
        df_tail["Source"] = base_name

        # 寫入 Sheet（Unsorted & Sorted）
        df_tail.to_excel(writer, index=False, sheet_name=f"{base_name}_Unsorted")
        df_sorted = df_tail.sort_values(by="TimeR (ns)").reset_index(drop=True)
        df_sorted.to_excel(writer, index=False, sheet_name=f"{base_name}_Sorted")

        # 存入字典供後續比對
        tail_dict[base_name] = df_tail

    # ----------------------------------------------------
    # 比對三者共同出現於 tail 的 packet_ID
    # ----------------------------------------------------
    try:
        set_pt = set(tail_dict["PT"]["packet_ID"])
        set_dbt = set(tail_dict["DBT"]["packet_ID"])
        set_kset = set(tail_dict["KSet"]["packet_ID"])

        common_ids = (set_pt & set_dbt) | (set_pt & set_kset) | (set_dbt & set_kset)

        # 輸出交集資訊
        print(f"✅ 三個模型共同的 tail packet_ID 數量：{len(common_ids)}")
        if common_ids:
            print("前 20 個共同 packet_ID：", sorted(list(common_ids))[:20])

        # 寫入 Excel Sheet
        df_common = pd.DataFrame(sorted(list(common_ids)), columns=["Common packet_ID"])
        df_common.to_excel(writer, index=False, sheet_name="Common_in_Tail")

    except KeyError as e:
        print(f"❌ 缺少模型資料，無法比對交集：{e}")

print(f"📄 所有結果已寫入 Excel：{output_path}")
