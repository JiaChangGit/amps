import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import KMeans
from sklearn.manifold import TSNE
import matplotlib.patheffects as pe
from scipy.stats import entropy, skew

# 參數設定
#FILE_PATH = "./part_0.txt"    # flow trace 檔案路徑（五欄位）
FILE_PATH = "./acl2_100k_trace"    # flow trace 檔案路徑（五欄位）
# 這份 trace 中，有 349,075 個獨特的五維 flow，各自對應一個出現次數 count。(acl2_100k_trace)
TOP_K = 20                         # Top-K 熱點 flow 數量
NUM_CLUSTERS = 7                  # 分群數量
MAX_TSNE_POINTS = 1000000          # t-SNE 最大抽樣點數
TOP_N_EACH_CLUSTER = 20           # 每群匯出前 N 大 flow

sns.set(style="whitegrid")
plt.rcParams["axes.titlesize"] = 14
plt.rcParams["axes.labelsize"] = 12

def load_trace(file_path):
    data = np.loadtxt(file_path, delimiter='\t', dtype=np.uint32, usecols=[0,1,2,3,4])
    df = pd.DataFrame(data, columns=['source_ip', 'destination_ip', 'source_port', 'destination_port', 'protocol'])
    grouped = df.groupby(df.columns.tolist()).size().reset_index(name='count')
    return grouped

def plot_topk_flows(df, k):
    topk = df.sort_values(by='count', ascending=False).head(k).copy()
    topk['flow'] = (topk['source_ip'].astype(str) + ':' + topk['source_port'].astype(str) +
                    ' > ' + topk['destination_ip'].astype(str) + ':' + topk['destination_port'].astype(str))
    plt.figure(figsize=(14, 6))
    sns.barplot(data=topk, x='flow', y='count', hue='count', palette='Reds', dodge=False, legend=False)
    plt.xticks(rotation=75, ha='right')
    plt.title(f"Top-{k} Most Frequent Flows")
    plt.xlabel("Flow (src > dst)")
    plt.ylabel("Count")
    plt.tight_layout()
    plt.savefig("topk_flows_verbose.png")
    plt.close()

def perform_clustering(df, n_clusters):
    scaler = StandardScaler()
    features = df[['source_ip', 'destination_ip', 'source_port', 'destination_port', 'protocol']]
    scaled = scaler.fit_transform(features)
    kmeans = KMeans(n_clusters=n_clusters, n_init=10, random_state=42)
    df['cluster'] = kmeans.fit_predict(scaled)
    return df, scaled, kmeans

def plot_tsne_clusters(df, scaled_data, kmeans, max_points=10000):
    if len(df) > max_points:
        df_sampled = df.sample(n=max_points, random_state=42).copy()
        scaled_sampled = scaled_data[df_sampled.index]
    else:
        df_sampled = df.copy()
        scaled_sampled = scaled_data

    all_data = np.vstack([scaled_sampled, kmeans.cluster_centers_])
    tsne = TSNE(n_components=2, perplexity=50, learning_rate='auto', init='pca', random_state=42)
    all_tsne = tsne.fit_transform(all_data)

    tsne_flows = all_tsne[:len(df_sampled)]
    tsne_centers = all_tsne[len(df_sampled):]

    df_sampled['tsne_x'] = tsne_flows[:, 0]
    df_sampled['tsne_y'] = tsne_flows[:, 1]

    palette = 'hsv' if df_sampled['cluster'].nunique() > 10 else 'tab10'
    plt.figure(figsize=(12, 8))
    sns.scatterplot(data=df_sampled, x='tsne_x', y='tsne_y', hue='cluster',
                    palette=palette, s=15, alpha=0.6, linewidth=0)
    plt.scatter(tsne_centers[:, 0], tsne_centers[:, 1], s=200, c='black', marker='X', label='Centroids')
    plt.title("t-SNE Projection with KMeans Cluster Centers")
    plt.legend()
    plt.tight_layout()
    plt.savefig("tsne_clusters_centroids.png")
    plt.close()

    return df_sampled

def export_cluster_profiles(df, top_n=10):
    os.makedirs("clusters_output", exist_ok=True)
    cluster_ids = sorted(df['cluster'].unique())

    for cluster_id in cluster_ids:
        cluster_df = df[df['cluster'] == cluster_id].copy()
        plt.figure(figsize=(10, 6))
        sns.scatterplot(data=cluster_df, x='tsne_x', y='tsne_y', hue='count', palette='Reds', s=20, legend=False)
        plt.title(f"Cluster {cluster_id} Flows (Size: {len(cluster_df)})")
        plt.xlabel("t-SNE X")
        plt.ylabel("t-SNE Y")

        top_flows = cluster_df.sort_values(by='count', ascending=False).head(top_n).copy()
        offsets = [(0, 12), (0, -12), (12, 0), (-12, 0), (8, 8), (-8, -8)]
        for i, (_, row) in enumerate(top_flows.iterrows()):
            label = f"{row['source_ip']}:{row['source_port']} > {row['destination_ip']}:{row['destination_port']}"
            dx, dy = offsets[i % len(offsets)]
            plt.annotate(
                label,
                (row['tsne_x'], row['tsne_y']),
                textcoords="offset points",
                xytext=(dx, dy),
                fontsize=9,
                fontweight='bold',
                color='black',
                ha='center',
                bbox=dict(boxstyle="round,pad=0.2", fc="white", alpha=0.75, edgecolor='gray'),
                path_effects=[pe.withStroke(linewidth=1, foreground="white")]
            )

        plt.tight_layout()
        plt.savefig(f"clusters_output/cluster_{cluster_id}_scatter.png")
        plt.close()

        top_flows['flow'] = (top_flows['source_ip'].astype(str) + ':' + top_flows['source_port'].astype(str) +
                             ' > ' + top_flows['destination_ip'].astype(str) + ':' + top_flows['destination_port'].astype(str))
        top_flows[['flow', 'count']].to_csv(f"clusters_output/cluster_{cluster_id}_top{top_n}.csv", index=False)

def plot_bucket_distribution(df, bucket_col='bucket', num_buckets=1024, title=''):
    bucket_counts = df[bucket_col].value_counts().sort_index()
    probs = bucket_counts.values / bucket_counts.sum()

    H = entropy(probs, base=2)
    max_entropy = np.log2(num_buckets)
    mean = bucket_counts.mean()
    stddev = bucket_counts.std()
    skw = skew(bucket_counts)

    plt.figure(figsize=(12, 5))
    sns.barplot(x=bucket_counts.index, y=bucket_counts.values, hue=bucket_counts.index, palette="crest", legend=False)
    plt.title(f"{title}Bucket Distribution\nEntropy: {H:.2f}/{max_entropy:.2f}, Mean: {mean:.1f}, Std: {stddev:.1f}, Skew: {skw:.2f}")
    plt.xlabel("Bucket ID")
    plt.ylabel("Flow Count")
    plt.tight_layout()
    safe_title = title.strip().replace(' ', '_').lower()
    plt.savefig(f"bucket_dist_{safe_title}.png")
    plt.close()

    print(f"\n[Bucket Stats] {title}")
    print(f"  - Buckets      : {len(bucket_counts)} / {num_buckets}")
    print(f"  - Total flows  : {bucket_counts.sum()}")
    print(f"  - Max          : {bucket_counts.max()}")
    print(f"  - Min          : {bucket_counts.min()}")
    print(f"  - Mean         : {mean:.2f}")
    print(f"  - StdDev       : {stddev:.2f}")
    print(f"  - Skewness     : {skw:.2f}")
    print(f"  - Entropy      : {H:.2f} bits / Max = {max_entropy:.2f} bits")

def murmur_style_hash(arr, seed=0):
    for i in range(5):
        h = int(arr[i]) & 0xFFFFFFFF
        h ^= h >> 33
        h = (h * 0xff51afd7ed558ccd) & 0xFFFFFFFFFFFFFFFF
        h ^= h >> 33
        h = (h * 0xc4ceb9fe1a85ec53) & 0xFFFFFFFFFFFFFFFF
        h ^= h >> 33
        seed ^= (h + 0x9e3779b9 + ((seed << 6) & 0xFFFFFFFFFFFFFFFF) + (seed >> 2))
    return seed
def plot_bitset_distribution_from_words(file_path, title='Bitset Distribution'):
    words_hex = []
    with open(file_path, 'r') as f:
        for line in f:
            if line.startswith("Word["):
                parts = line.strip().split()
                hex_val = int(parts[2], 16)
                words_hex.append(hex_val)

    bit_array = np.zeros(1024, dtype=np.uint8)
    for i, word in enumerate(words_hex):
        for b in range(64):
            if word & (1 << b):
                bit_array[i * 64 + b] = 1

    total_set = np.sum(bit_array)
    ratio = total_set / 1024 * 100

    plt.figure(figsize=(20, 4))
    plt.bar(range(1024), bit_array, width=1, color='steelblue', edgecolor='none')
    plt.title(f"{title}\nSet bits: {total_set}/1024 ({ratio:.2f}%)")
    plt.xlabel("Bit Index")
    plt.ylabel("Set")
    plt.tight_layout()
    plt.savefig("bitset_distribution.png")
    plt.close()

    print(f"[Bitset File] {file_path}")
    print(f"  - Set bits : {total_set} / 1024")
    print(f"  - Usage    : {ratio:.2f}%")
def main():
    np.random.seed(42)
    df = load_trace(FILE_PATH)
    plot_topk_flows(df, TOP_K)

    #df['bucket'] = df.apply(lambda row: murmur_style_hash([row['source_ip'], row['destination_ip'],row['source_port'], row['destination_port'],row['protocol']]) % 1024, axis=1)

    #plot_bucket_distribution(df, bucket_col='bucket', num_buckets=1024, title='MyCustomHash')
    #plot_bitset_distribution_from_words("bloom_filter_dbt.txt", title="Bitmap Buckets")

    df_clustered, scaled_data, kmeans = perform_clustering(df, NUM_CLUSTERS)
    df_tsne = plot_tsne_clusters(df_clustered, scaled_data, kmeans, MAX_TSNE_POINTS)
    export_cluster_profiles(df_tsne, TOP_N_EACH_CLUSTER)

if __name__ == "__main__":
    main()
