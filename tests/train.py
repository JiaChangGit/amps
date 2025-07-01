# ============================ [環境設定與預設值] ============================
import os
import logging

# 設置日誌格式，方便追蹤程式執行
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# 禁用 GPU，強制使用 CPU 訓練（可根據需求移除）
os.environ["CUDA_VISIBLE_DEVICES"] = ""
# 抑制 Python 棄用警告（謹慎使用，可能隱藏重要訊息）
os.environ["PYTHONWARNINGS"] = "ignore::DeprecationWarning"
# 抑制 TensorFlow 日誌（僅在需要 TensorFlow 時保留）
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"

# ============================ [解析終端參數] ============================
import argparse
parser = argparse.ArgumentParser(description="RL PPO Training for Packet Classifier")
parser.add_argument("--rules", required=True, help="Path to ruleset file")
parser.add_argument("--trace", required=True, help="Path to packet trace file")
args = parser.parse_args()

# ============================ [必要套件導入] ============================
import ray
from ray.rllib.algorithms.ppo import PPOConfig
from ray.tune.registry import register_env
from rl_environment import ComplementaryObjectsEnv

# 註冊自定義環境，供 RLlib 使用
register_env("ComplementaryObjectsEnv", lambda cfg: ComplementaryObjectsEnv(**cfg))

# ============================ [初始化 Ray] ============================
try:
    ray.init(logging_level="ERROR")
    logging.info("✅ Ray 初始化成功")
except Exception as e:
    logging.error(f"❌ Ray 初始化失敗：{e}")
    exit(1)

# ============================ [設置 TensorBoard 日誌] ============================
log_dir = "./ray_results/ppo_tensorboard"
try:
    os.makedirs(log_dir, exist_ok=True)
    logging.info(f"✅ TensorBoard 日誌目錄創建成功：{log_dir}")
except Exception as e:
    logging.error(f"❌ 無法創建 TensorBoard 日誌目錄：{e}")
    exit(1)

# ============================ [PPO 訓練設定] ============================
config = PPOConfig()\
    .environment(
        env="ComplementaryObjectsEnv",
        env_config={
            "set_field_dim": 3,
            "max_binth": 32,
            "max_top_k": 8,
            "max_c_bound": 64,
            "max_threshold": 32,
            "rules_file": args.rules,
            "packets_file": args.trace
        }
    )\
    .framework("torch")\
    .resources(num_gpus=0)\
    .env_runners(num_env_runners=2)\
    .training(
        lr=3e-4,
        train_batch_size=128,
        model={"fcnet_hiddens": [256, 256]}
    )\
    .api_stack(
        enable_rl_module_and_learner=False,
        enable_env_runner_and_connector_v2=False
    )\
    .experimental(_disable_preprocessor_api=True)

# 設定 TensorBoard logger
config.logger_config = {
    "type": "ray.tune.logger.UnifiedLogger",
    "logdir": log_dir,
}

# ============================ [建立 PPO 演算法實例] ============================
try:
    algorithm = config.build()
except Exception as e:
    print(f"❌ 錯誤：無法建立 PPO 演算法 - {e}")
    ray.shutdown()
    exit(1)

# ============================ [訓練流程（多輪）] ============================
for i in range(2):
    try:
        result = algorithm.train()
        print(f"\n[訓練迭代 {result['training_iteration']}]")
    except Exception as e:
        print(f"❌ 訓練失敗：{e}")
        break

# ============================ [儲存 PPO 模型 checkpoint] ============================
checkpoint_dir = "./checkpoints"
os.makedirs(checkpoint_dir, exist_ok=True)

try:
    checkpoint_path = algorithm.save(checkpoint_dir=checkpoint_dir)
    print(f"\n✅ 模型已保存於：{checkpoint_path}")
except Exception as e:
    print(f"❌ 儲存失敗：{e}")

# ============================ [推論測試流程] ============================
env = ComplementaryObjectsEnv(
    set_field_dim=3,
    max_binth=32,
    max_top_k=8,
    max_c_bound=64,
    max_threshold=32,
    rules_file=args.rules,
    packets_file=args.trace
)

obs, _ = env.reset()
done = False
step = 0

print("\n--- 開始推論 ---")
rewards = []
while not done:
    try:
        action = algorithm.compute_single_action(obs)
        obs, reward, done, truncated, _ = env.step(action)
        rewards.append(reward)
        env.render()
        print(f"➡️ 第 {step+1} 步 reward：{reward:.2f}")
        step += 1
        if done or truncated:
            break
    except Exception as e:
        print(f"❌ 推論失敗：{e}")
        env.render()
        break

print("\n推論結果（每步 reward）：")
print(rewards)
with open("./INFO/rewards_values.txt", "w") as f:
    for r in rewards:
        f.write(f"{r}\n")

# ============================ [關閉 Ray] ============================
ray.shutdown()
