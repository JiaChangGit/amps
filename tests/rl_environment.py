import gymnasium as gym
import numpy as np
from gymnasium import spaces
from rl_gym import RLGym, PT_Object, DBT_Object, DT_Object
import os
import random

class ComplementaryObjectsEnv(gym.Env):
    """
    自訂強化學習環境，實現三步驟訓練：
    - 第 1 步：設置 PT，reward 為 PT 的 99th 分位數（負值）。
    - 第 2 步：設置 DBT，reward 為 PT 與 DBT 慢速封包交集大小（負值）。
    - 第 3 步：設置 DT，reward 為 PT、DBT 和 DT 三者慢速封包交集大小（負值）。
    - 若第三步 reward 達到 -5 或更好，提前結束 episode。
    - 目標：最小化分類成本（負 reward）。
    """

    def __init__(self, set_field_dim=3, max_binth=32, max_top_k=8, max_c_bound=64, max_threshold=32, rules_file=None,
                 packets_file=None):
        """
        初始化環境，定義動作空間與觀察空間。

        參數：
        - set_field_dim：PT 欄位維度（預設 3）。
        - max_binth：DBT binth 上限。
        - max_top_k：DBT top-k 上限。
        - max_c_bound：DBT c_bound 上限。
        - max_threshold：DT threshold 上限。
        - rules_file：規則檔案路徑。
        - packets_file：封包檔案路徑。
        """
        super(ComplementaryObjectsEnv, self).__init__()

        self.set_field_dim = set_field_dim
        self.max_binth = max_binth
        self.max_top_k = max_top_k
        self.max_c_bound = max_c_bound
        self.max_threshold = max_threshold
        self.builder = RLGym()

        self.rules_file = rules_file
        self.packets_file = packets_file

        # 檢查檔案存在
        if not os.path.exists(self.rules_file):
            raise FileNotFoundError(f"規則檔案未找到：{self.rules_file}")
        if not os.path.exists(self.packets_file):
            raise FileNotFoundError(f"封包檔案未找到：{self.packets_file}")

        # 載入規則與封包檔案
        try:
            self.builder.load_KSet_rule_packets(self.rules_file, self.packets_file)
            print(f"成功載入檔案：rules={self.rules_file}, packets={self.packets_file}")
        except Exception as e:
            print(f"錯誤：無法載入規則或封包檔案 - {e}")
            raise RuntimeError(f"Failed to load rule/packet files: {e}")

        # --------------------- 定義動作空間（分三步） ---------------------
        # 第 1 步：PT 參數
        self.action_space_pt = spaces.Dict({
            "set_field": spaces.MultiDiscrete([8] * self.set_field_dim),  # 選擇三個欄位索引（0~7）
            "set_port": spaces.Discrete(2)  # 是否加入 port 欄位（0 或 1）
        })
        # 第 2 步：DBT 參數
        self.action_space_dbt = spaces.Dict({
            "binth": spaces.Discrete(max_binth - 3),  # [4, max_binth]
            "end_bound": spaces.Box(low=0.1, high=0.9, shape=(1,), dtype=np.float32),
            "top_k": spaces.Discrete(max_top_k),  # [1, max_top_k]
            "c_bound": spaces.Discrete(max_c_bound - 6)  # [7, max_c_bound]
        })
        # 第 3 步：DT 參數
        self.action_space_dt = spaces.Dict({
            "threshold": spaces.Discrete(max_threshold - 6),  # [7, max_threshold]
            "is_prefix_5d": spaces.Discrete(2)  # 是否採用 5 維前綴
        })

        # 為了兼容 RLlib，定義一個完整的動作空間（用於初始化）
        self.action_space = spaces.Dict({
            "set_field": spaces.MultiDiscrete([8] * self.set_field_dim),
            "set_port": spaces.Discrete(2),
            "binth": spaces.Discrete(max_binth - 3),
            "end_bound": spaces.Box(low=0.1, high=0.9, shape=(1,), dtype=np.float32),
            "top_k": spaces.Discrete(max_top_k),
            "c_bound": spaces.Discrete(max_c_bound - 6),
            "threshold": spaces.Discrete(max_threshold - 6),
            "is_prefix_5d": spaces.Discrete(2)
        })

        # --------------------- 定義觀察空間 ---------------------
        # 維度：set_field(3) + set_port(1) + DBT(4) + DT(2) = 10 維，正規化到 [0,1]
        obs_dim = set_field_dim + 1 + 4 + 2
        self.observation_space = spaces.Box(low=0.0, high=1.0, shape=(obs_dim,), dtype=np.float32)

        # 初始化狀態
        self.reset()

    def reset(self, seed=None, options=None):
        """
        重置環境，初始化 PT、DBT 和 DT 物件為 None，並設置步驟計數為 0。

        參數：
        - seed：隨機種子。
        - options：其他選項（未使用）。

        返回：
        - 觀察值（10 維向量）。
        - 空資訊字典。
        """
        super().reset(seed=seed)
        if seed is not None:
            np.random.seed(seed)
            random.seed(seed)

        # 初始化物件
        self.obj_pt = None
        self.obj_dbt = None
        self.obj_dt = None
        self.current_step = 0

        return self._get_obs(), {}

    def _get_obs(self):
        """
        取得當前觀察值，正規化到 [0,1]。未建立的物件參數以 0 填充。

        返回：
        - 10 維向量，包含 PT、DBT 和 DT 的正規化參數。
        """
        pt_field = np.array(self.obj_pt.get_set_field() if self.obj_pt else [0] * self.set_field_dim, dtype=np.float32) / 7.0
        pt_port = np.array([self.obj_pt.get_set_port() if self.obj_pt else 0], dtype=np.float32)

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
        """
        執行一步，根據 current_step 處理 PT、DBT 或 DT 的設置，並計算對應 reward。
        若第三步 reward 達到 -5 或更好，提前結束 episode。

        參數：
        - action：動作字典，包含當前步驟的參數。

        返回：
        - 觀察值、reward、done、truncated、資訊字典。
        """
        reward = -10000000.0  # 預設懲罰值（動作無效時）
        done = False
        truncated = False

        # 根據步驟選擇動作空間
        action_space = [self.action_space_pt, self.action_space_dbt, self.action_space_dt][self.current_step]

        # 驗證動作是否有效
        if not self.action_space.contains(action):
            print(f"無效動作（步驟 {self.current_step}）：{action}")
            return self._get_obs(), reward, True, True, {}

        # 將動作映射為實際值
        action_mapped = {
            "set_field": action.get("set_field", [0] * self.set_field_dim).tolist(),
            "set_port": bool(action.get("set_port", 0)),
            "binth": action.get("binth", 0) + 4,
            "end_bound": action.get("end_bound", [0.1])[0],
            "top_k": action.get("top_k", 0) + 1,
            "c_bound": action.get("c_bound", 0) + 7,
            "threshold": action.get("threshold", 0) + 7,
            "is_prefix_5d": bool(action.get("is_prefix_5d", 0))
        }

        # 驗證 set_field 不重複
        if self.current_step == 0 and len(set(action_mapped["set_field"])) != self.set_field_dim:
            print(f"set_field 重複：{action_mapped['set_field']}，隨機重置")
            action_mapped["set_field"] = random.sample(range(8), self.set_field_dim)

        # 驗證 end_bound 範圍
        if self.current_step == 1 and not (0.1 <= action_mapped["end_bound"] <= 0.9):
            print(f"end_bound 超出範圍：{action_mapped['end_bound']}")
            return self._get_obs(), reward, True, True, {}

        # 根據步驟執行邏輯
        if self.current_step == 0:
            # 第 1 步：設置 PT
            try:
                self.obj_pt = self.builder.create_pt_first(action_mapped["set_field"], action_mapped["set_port"])
                print(f"步驟 1 PT 物件：set_field={action_mapped['set_field']}, set_port={action_mapped['set_port']}")
                pt_score = self.builder.evaluate_pt(self.obj_pt)
                reward = -pt_score  # 負值，最小化 99th 分位數
                print(f"PT reward (99th)：{reward}")
                self.current_step += 1
            except Exception as e:
                print(f"無法建立 PT：{e}")
                return self._get_obs(), reward, True, True, {}

        elif self.current_step == 1:
            # 第 2 步：設置 DBT
            try:
                self.obj_dbt = self.builder.create_dbt_object(
                    action_mapped["binth"],
                    action_mapped["end_bound"],
                    action_mapped["top_k"],
                    action_mapped["c_bound"],
                    self.obj_pt
                )
                dbt_score = self.builder.evaluate_dbt(self.obj_pt, self.obj_dbt)
                reward = -dbt_score  # 負值，最小化交集大小
                print(f"PT ∩ DBT reward：{reward}")
                self.current_step += 1
            except Exception as e:
                print(f"無法建立 DBT：{e}")
                return self._get_obs(), reward, True, True, {}

        elif self.current_step == 2:
            # 第 3 步：設置 DT
            try:
                self.obj_dt = self.builder.create_dt_object(
                    action_mapped["threshold"],
                    action_mapped["is_prefix_5d"],
                    self.obj_pt,
                    self.obj_dbt
                )
                dt_score = self.builder.evaluate_dt(self.obj_pt, self.obj_dbt, self.obj_dt)
                reward = -dt_score  # 負值，最小化三者交集大小
                print(f"PT ∩ DBT ∩ DT reward：{reward}")

                # 檢查 reward 是否達到 -5 或更好
                if reward >= -5:
                    print(f"Reward {reward} >= -5，提前結束 episode")
                    done = True
                else:
                    done = True  # 第三步總是結束 episode

                # 寫入參數到 param.txt
                os.makedirs(os.path.join(".", "INFO"), exist_ok=True)
                with open(os.path.join(".", "INFO", "param.txt"), "w") as f:
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
        """
        顯示當前狀態，用於 debug。

        參數：
        - mode：顯示模式（預設為 'human'）。
        """
        print(f"Step {self.current_step}:")
        if self.obj_pt:
            print(f"PT_Object: set_field={self.obj_pt.get_set_field()}, set_port={self.obj_pt.get_set_port()}")
        if self.obj_dbt:
            print(f"DBT_Object: binth={self.obj_dbt.get_binth()}, end_bound={self.obj_dbt.get_end_bound()}, "
                  f"top_k={self.obj_dbt.get_top_k()}, c_bound={self.obj_dbt.get_c_bound()}")
        if self.obj_dt:
            print(f"DT_Object: threshold={self.obj_dt.get_threshold()}, "
                  f"is_prefix_5d={self.obj_dt.get_is_prefix_5d()}")
