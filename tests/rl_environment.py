import gymnasium as gym
import numpy as np
from gymnasium import spaces
from rl_gym import RLGym, PT_Object, DBT_Object, DT_Object  # 導入三個 C++ 封包分類器物件
import os
import random

class ComplementaryObjectsEnv(gym.Env):
    """
    自訂強化學習環境：
    - 每個 episode 分兩步：先選擇 PT + DBT，然後選擇 DT 參數。
    - 根據三種資料結構（PT → DBT → DT）參數設定，整體 reward = -score，目標是最小化分類成本。
    """

    def __init__(self, set_field_dim=3, max_binth=32, max_top_k=8, max_c_bound=64, max_threshold=32):
        """
        初始化環境與動作空間、觀察空間等。

        set_field_dim：三個欄位維度，用來挑選資料欄位。
        max_binth：DBT 中 binth 上限值。
        max_top_k：DBT 中 top-k 上限值。
        max_c_bound：DBT 中 c_bound 上限。
        max_threshold：DT 中 threshold 上限。
        """
        super(ComplementaryObjectsEnv, self).__init__()

        self.set_field_dim = set_field_dim
        self.max_binth = max_binth
        self.max_top_k = max_top_k
        self.max_c_bound = max_c_bound
        self.max_threshold = max_threshold
        self.builder = RLGym()  # 綁定 C++ 模組的包裝器物件

        # 指定使用的 ClassBench 規則集與封包追蹤集
        self.rules_file = "../../classbench_set/ipv4-ruleset/acl1_100k"
        self.packets_file = "../../classbench_set/ipv4-trace/acl1_100k_trace"

        # 確認檔案存在，否則直接丟出錯誤
        if not os.path.exists(self.rules_file):
            raise FileNotFoundError(f"規則檔案未找到：{self.rules_file}")
        if not os.path.exists(self.packets_file):
            raise FileNotFoundError(f"封包檔案未找到：{self.packets_file}")

        # 從 C++ Builder 中載入規則與封包檔案，並初始化基礎資料
        try:
            self.builder.load_KSet_rule_packets(self.rules_file, self.packets_file)
            print(f"成功載入檔案：rules={self.rules_file}, packets={self.packets_file}")
        except Exception as e:
            print(f"錯誤：無法載入規則或封包檔案 - {e}")
            raise RuntimeError(f"Failed to load rule/packet files: {e}")

        # --------------------- 定義動作空間 ---------------------
        self.action_space = spaces.Dict({
            "set_field": spaces.MultiDiscrete([8] * self.set_field_dim),  # 選擇三個欄位索引（0~7），不應重複
            "set_port": spaces.Discrete(2),  # 是否加入 port 欄位（bool）
            "binth": spaces.Discrete(max_binth - 3),  # 實際值會 +4，使其範圍變成 [4, max_binth]
            "end_bound": spaces.Box(low=0.1, high=0.9, shape=(1,), dtype=np.float32),  # float 參數
            "top_k": spaces.Discrete(max_top_k),  # 實際值會 +1 → [1, max_top_k]
            "c_bound": spaces.Discrete(max_c_bound - 6),  # 實際值會 +7 → [7, max_c_bound]
            "threshold": spaces.Discrete(max_threshold - 6),  # 實際值會 +7 → [7, max_threshold]
            "is_prefix_5d": spaces.Discrete(2)  # 是否採用 5 維前綴，影響 decision tree 建立方式
        })

        # --------------------- 定義觀察空間 ---------------------
        # 維度：set_field(3) + set_port(1) + DBT(4) + DT(2) = 10 維向量，數值皆正規化為 [0,1]
        obs_dim = set_field_dim + 1 + 4 + 2
        self.observation_space = spaces.Box(low=0.0, high=1.0, shape=(obs_dim,), dtype=np.float32)

        # 執行第一次 reset（初始化狀態）
        self.reset()

    def reset(self, seed=None, options=None):
        """重置環境，每個 episode 的開頭都會重設 PT（欄位 + port），並清空 DBT/DT 狀態"""
        super().reset(seed=seed)
        if seed is not None:
            np.random.seed(seed)
            random.seed(seed)

        # 產生不重複的三個欄位索引（0~7）
        set_field = random.sample(range(8), self.set_field_dim)
        set_port = np.random.randint(0, 2)

        try:
            self.obj_pt = self.builder.create_pt_first()
            self.obj_pt.set_all_params(set_field, set_port)
            print(f"重置 PT 物件：set_field={set_field}, set_port={set_port}")
        except Exception as e:
            raise RuntimeError(f"Failed to create/set PT object: {e}")

        self.obj_dbt = None  # 初始時尚未建立 DBT
        self.obj_dt = None   # 初始時尚未建立 DT
        self.current_step = 0

        return self._get_obs(), {}

    def _get_obs(self):
        """取得目前觀察值，正規化為 [0,1] 範圍，若尚未建立對應物件則以 0 補齊"""
        pt_field = np.array(self.obj_pt.get_set_field(), dtype=np.float32) / 7.0
        pt_port = np.array([self.obj_pt.get_set_port()], dtype=np.float32)

        dbt_params = np.array([
            (self.obj_dbt.get_binth() if self.obj_dbt else 0.0) / 32.0,
            (self.obj_dbt.get_end_bound() if self.obj_dbt else 0.0) / 0.9,
            (self.obj_dbt.get_top_k() if self.obj_dbt else 0.0) / 8.0,
            (self.obj_dbt.get_c_bound() if self.obj_dbt else 0.0) / 64.0
        ], dtype=np.float32)

        dt_params = np.array([
            (self.obj_dt.get_threshold() if self.obj_dt else 0.0) / 32.0,
            (self.obj_dt.get_is_prefix_5d() if self.obj_dt else 0.0)
        ], dtype=np.float32)

        return np.concatenate([pt_field, pt_port, dbt_params, dt_params])

    def step(self, action):
        """執行一個時間步，根據輸入的 action 建立 DBT 或 DT，並計算 reward（為負分數）"""
        reward = -100000.0
        done = False
        truncated = False

        if not self.action_space.contains(action):
            print(f"無效動作：{action}")
            return self._get_obs(), reward, True, True, {}

        # 將動作映射為實際值
        action_mapped = {
            "set_field": action["set_field"].tolist(),
            "set_port": bool(action["set_port"]),
            "binth": action["binth"] + 4,
            "end_bound": action["end_bound"][0],
            "top_k": action["top_k"] + 1,
            "c_bound": action["c_bound"] + 7,
            "threshold": action["threshold"] + 7,
            "is_prefix_5d": bool(action["is_prefix_5d"])
        }

        if len(set(action_mapped["set_field"])) != self.set_field_dim:
            print(f"set_field 重複：{action_mapped['set_field']}，隨機重置")
            action_mapped["set_field"] = random.sample(range(8), self.set_field_dim)

        if not (0.1 <= action_mapped["end_bound"] <= 0.9):
            print(f"end_bound 超出範圍：{action_mapped['end_bound']}")
            return self._get_obs(), reward, True, True, {}

        # 更新 PT 設定
        try:
            self.obj_pt = self.builder.create_pt_first()
            self.obj_pt.set_all_params(action_mapped["set_field"], action_mapped["set_port"])
            print(f"步驟 PT 物件：set_field={action_mapped['set_field']}, set_port={action_mapped['set_port']}")
        except Exception as e:
            return self._get_obs(), reward, True, True, {}

        # 根據目前 step 進行 DBT 或 DT 建立
        if self.current_step == 0:
            try:
                self.obj_dbt = self.builder.create_dbt_object(
                    action_mapped["binth"],
                    action_mapped["end_bound"],
                    action_mapped["top_k"],
                    action_mapped["c_bound"],
                    self.obj_pt
                )
                reward = -self.builder.evaluate_dbt(self.obj_pt, self.obj_dbt)
                self.current_step += 1
            except Exception as e:
                print(f"無法建立 DBT：{e}")
                return self._get_obs(), reward, True, True, {}
        else:
            try:
                self.obj_dt = self.builder.create_dt_object(
                    action_mapped["threshold"],
                    action_mapped["is_prefix_5d"],
                    self.obj_pt,
                    self.obj_dbt
                )
                reward = -self.builder.evaluate_dt(self.obj_pt, self.obj_dbt, self.obj_dt)
                done = True  # 第二步結束，episode 完成

                # 寫入參數供後續分析（INFO/param.txt）
                os.makedirs(os.path.join("..", "INFO"), exist_ok=True)
                with open(os.path.join("..", "INFO", "param.txt"), "w") as f:
                    f.write(f"PT_set_field={' '.join(map(str, self.obj_pt.get_set_field()))}\n")
                    f.write(f"PT_set_port={self.obj_pt.get_set_port()}\n")
                    f.write(f"DBT_binth={self.obj_dbt.get_binth()}\n")
                    f.write(f"DBT_end_bound={self.obj_dbt.get_end_bound():.1f}\n")
                    f.write(f"DBT_top_k={self.obj_dbt.get_top_k()}\n")
                    f.write(f"DBT_c_bound={self.obj_dbt.get_c_bound()}\n")
                    f.write(f"DT_threshold={self.obj_dt.get_threshold()}\n")
                    f.write(f"DT_is_prefix_5d={self.obj_dt.get_is_prefix_5d()}\n")
            except Exception as e:
                print(f"無法建立 DT：{e}")
                return self._get_obs(), reward, True, True, {}

        return self._get_obs(), reward, done, truncated, {}

    def render(self, mode='human'):
        """顯示目前狀態，方便 debug 或分析策略效果"""
        print(f"Step {self.current_step}:")
        print(f"PT_Object: set_field={self.obj_pt.get_set_field()}, set_port={self.obj_pt.get_set_port()}")
        if self.obj_dbt:
            print(f"DBT_Object: binth={self.obj_dbt.get_binth()}, end_bound={self.obj_dbt.get_end_bound()}, "
                  f"top_k={self.obj_dbt.get_top_k()}, c_bound={self.obj_dbt.get_c_bound()}")
        if self.obj_dt:
            print(f"DT_Object: threshold={self.obj_dt.get_threshold()}, "
                  f"is_prefix_5d={self.obj_dt.get_is_prefix_5d()}")
