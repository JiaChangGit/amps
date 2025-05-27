import pandas as pd
import matplotlib.pyplot as plt

# === 步驟 1: 讀取封包檔案 ===
 # 請改為你本地路徑，例如："C:/Users/你/Downloads/acl1_1k_trace.txt"
file_path = "acl1_100k_trace"

# df = pd.read_csv(file_path, delim_whitespace=True, header=None, usecols=[0,1,2,3,4])
df = pd.read_csv(file_path, sep="\t", header=None, usecols=[0,1,2,3,4])
df.columns = ["src_ip", "dst_ip", "src_port", "dst_port", "protocol"]

# === 步驟 2: 唯一值統計 ===
summary_df = pd.DataFrame({
    "Field": ["src_ip", "dst_ip", "src_port", "dst_port", "protocol", "5-tuple packet"],
    "Unique Count": [
        df["src_ip"].nunique(),
        df["dst_ip"].nunique(),
        df["src_port"].nunique(),
        df["dst_port"].nunique(),
        df["protocol"].nunique(),
        df.drop_duplicates().shape[0]
    ]
})

print("=== 唯一值統計 ===")
print(summary_df.to_string(index=False))

# === 步驟 3: 封包出現次數 + 排名（rank-frequency） ===
packet_counts = df.value_counts().reset_index(name="count")
packet_counts["rank"] = packet_counts["count"].rank(method="first", ascending=False).astype(int)
packet_counts_sorted = packet_counts.sort_values(by="rank")

# === 步驟 4: Rank-Frequency Plot（雙對數） ===
plt.figure(figsize=(10, 6))
plt.plot(packet_counts_sorted["rank"], packet_counts_sorted["count"],
         marker='o', linestyle='-', alpha=0.8)
plt.xscale("log")
plt.yscale("log")
plt.xlabel("Rank (log scale)")
plt.ylabel("Frequency (log scale)")
plt.title("Rank-Frequency Plot of Packets (5-tuple)")
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.tight_layout()
plt.show()

# === 步驟 5: Top 20 最常見封包列印 ===
top_20 = packet_counts_sorted.head(20)
print("\n=== Top 20 最常見的 5-tuple 封包 ===")
print(top_20.to_string(index=False))

# === （可選）儲存結果 ===
# summary_df.to_csv("uniq_summary.csv", index=False)
# packet_counts_sorted.to_csv("packet_rank_freq.csv", index=False)
# top_20.to_csv("top20_packets.csv", index=False)
