#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
配置服务（FastAPI + ROS）
- 监听端口：12348
- 路径：POST /frame
- 职责：
  * 接收前端保存的配置（openai_key、speaker、languageIndex、tableRows 等）
  * 将 languageIndex 映射为 ROS 参数 /language=en/jk
  * 设置 ROS 参数 /talker_openai、/qianwenKey、/openaiKey 等
  * 将 tableRows 写入 YAML 文件并加载到 /knowledge 命名空间（覆盖式，若本次无有效条目则不覆盖）
"""

import os
import subprocess
import yaml
from typing import Any, Dict, List

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware

# ---- ROS 初始化（允许在独立进程中设置 rosparam）----
try:
    import rospy
    ROS_AVAILABLE = True
except Exception:
    ROS_AVAILABLE = False

def ensure_rospy_inited():
    if ROS_AVAILABLE and not rospy.core.is_initialized():
        try:
            rospy.init_node("config_service", anonymous=True, disable_signals=True)
        except Exception:
            pass

def log_info(msg: str):
    if ROS_AVAILABLE:
        try:
            rospy.loginfo(msg)
            return
        except Exception:
            pass
    print(msg)

def log_warn(msg: str):
    if ROS_AVAILABLE:
        try:
            rospy.logwarn(msg)
            return
        except Exception:
            pass
    print(f"[WARN] {msg}")

def set_ros_param(key: str, value: Any):
    if ROS_AVAILABLE:
        try:
            rospy.set_param(key, value)
        except Exception as e:
            log_warn(f"设置 ROS 参数失败 {key}={value}: {e}")
    else:
        log_info(f"[no-ROS] set_param {key}={value}")

# ---- 知识库文件与命名空间（按你的路径）----
KNOWLEDGE_FILE_PATH = "/root/unitree_2025_08_14/src/L2/config/yoyo_knowledge.yaml"
KNOWLEDGE_NS = "/knowledge"

# ---- YAML 写入工具：键统一加引号，值用 block scalar | 保留多行 ----
def yaml_escape_key(key: str) -> str:
    """
    为安全起见，统一用双引号包裹键，避免阿语、空格、问号、冒号等字符导致 YAML 解析问题。
    """
    k = str(key).replace('"', '\\"')
    return f'"{k}"'

def get_knowledge_config(mid_dict: Dict[str, Any]) -> str:
    """
    将一行 tableRows 写成 YAML：
    - 不再写 action:<index>，始终把 speechValue 写为多行字符串块
    - key: |  (多行字符串)
        <缩进的内容>
    返回空字符串表示该行无效（例如 key 为空）
    """
    raw_key = str(mid_dict.get("col1", "")).strip()
    if not raw_key:
        return ""  # key 为空则视为无效条目

    key = yaml_escape_key(raw_key)

    # 值始终取 speechValue（允许为空；空时也写成空块以保持一致）
    content = str(mid_dict.get("speechValue", ""))
    content = content.rstrip("\n")

    if content == "":
        return f"{key}: |\n"

    lines = content.splitlines()
    indented = "\n".join(f"  {line}" if line else "  " for line in lines)
    return f"{key}: |\n{indented}\n"

def write_knowledge_and_load(rows: List[Dict[str, Any]]) -> bool:
    # 生成块并统计有效条目
    blocks: List[str] = []
    try:
        for elem in rows or []:
            block = get_knowledge_config(elem)
            if block:
                blocks.append(block)
    except Exception as e:
        log_warn(f"处理 rows 出错: {e}")

    # 若本次没有任何有效条目（全部 key 为空或 rows 为空），则不覆盖 /knowledge
    if len(blocks) == 0:
        log_info("本次未收到任何有效知识库条目（key 为空或无数据），跳过覆盖，保留现有 /knowledge")
        return True  # 按成功返回，表示服务无异常

    # 确保目录存在
    try:
        os.makedirs(os.path.dirname(KNOWLEDGE_FILE_PATH), exist_ok=True)
    except Exception as e:
        log_warn(f"创建目录失败: {e}")

    # 写文件（使用 UTF-8）
    try:
        with open(KNOWLEDGE_FILE_PATH, "w", encoding="utf-8") as f:
            for block in blocks:
                f.write(block)
        with open(KNOWLEDGE_FILE_PATH, "a", encoding="utf-8") as f:
            f.write("\n")
    except Exception as e:
        log_warn(f"写入知识库文件失败: {e}")
        return False

    # 覆盖式加载到 ROS 参数：先删除命名空间，再加载
    try:
        try:
            subprocess.check_call(["rosparam", "delete", KNOWLEDGE_NS])
            log_info(f"已删除旧的参数命名空间 {KNOWLEDGE_NS}")
        except subprocess.CalledProcessError as e:
            log_warn(f"删除命名空间时可能不存在或失败，忽略：{e}")

        subprocess.check_call(["rosparam", "load", KNOWLEDGE_FILE_PATH, KNOWLEDGE_NS])
        log_info(f"知识库已覆盖并加载到 {KNOWLEDGE_NS}（共 {len(blocks)} 条）")
        return True
    except Exception as e:
        log_warn(f"覆盖加载知识库失败: {e}")
        return False

# ---- 语言映射工具：只允许 en/jk，其他一律回退 en ----
def map_language(payload: Dict[str, Any]) -> str:
    lang_value = "en"
    if "languageIndex" in payload:
        try:
            idx = int(payload.get("languageIndex"))
        except Exception:
            idx = 0
        # 0: en, 1: jk, 
        lang_value = {0: "en", 1: "jk", 2: "jk"}.get(idx, "en")
    else:
        lang_text = str(payload.get("language", "")).strip()
        if lang_text == "jk":
            lang_value = "jk"
        else:
            lang_value = "en"
    return "en" if lang_value not in ("en", "jk") else lang_value

# ---- FastAPI 应用 ----
app = FastAPI(title="rospy-config", version="0.1.6")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 按需限制
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
async def root():
    return {"status": "running", "service": "config_service"}

def extract_openai_key(payload: Dict[str, Any]) -> str:
    """
    统一获取前端传入的 OpenAI Key，仅接受 openai_key 字段。
    """
    return payload.get("openai_key", "")

@app.post("/frame")
async def frame_handler(req: Request):
    """
    接收前端保存设置的 POST：
    {
      "qianwenKey": "...",
      "openai_key": "...",        // 仅此字段
      "speaker": "nova",
      "languageIndex": 0,        
      "tableRows": [ {col1, actionTypeIndex, actionIndex, speechValue}, ... ],
      "volume": "...", "mic": "...", "speakerDevice": "..."
    }
    兼容旧字段 "language": "英语/阿拉伯语"
    """
    ensure_rospy_inited()

    try:
        payload = await req.json()
    except Exception as e:
        log_warn(f"JSON 解析失败: {e}")
        return {"ok": False, "error": "bad json"}

    # 1) 设置密钥
    qianwen_key_value = payload.get("openai_key", "")
    if qianwen_key_value:
        set_ros_param("openai_key", qianwen_key_value)

    # 仅接受 openai_key 字段；ROS 参数保持 /openaiKey
    openai_key_value = extract_openai_key(payload)
    set_ros_param("openai_key", openai_key_value)

    lang_value = map_language(payload)
    set_ros_param("language", lang_value)

    # 3) OpenAI 发音人（speaker）
    set_ros_param("speaker", payload.get("speaker", "Echo"))

    # 4) 其他参数（可选）
    set_ros_param("talker_jk", payload.get("talker_jk", ""))
    set_ros_param("volume", payload.get("volume", ""))
    set_ros_param("mic", payload.get("mic", ""))
    set_ros_param("speakerDevice", payload.get("speakerDevice", ""))

    # 5) 写知识库并覆盖加载（当本次没有任何有效条目时，不覆盖，保留现有 /knowledge）
    rows = payload.get("tableRows", [])
    loaded = write_knowledge_and_load(rows)

    return {
        "ok": True,
        "language": lang_value,
        "knowledgeLoaded": loaded,
        "has_openai_key": bool(openai_key_value),
    }

# ---- 主入口 ----
if __name__ == "__main__":
    # 以 uvicorn 方式运行：python config_service.py
    import uvicorn
    host = os.environ.get("CFG_HOST", "0.0.0.0")
    port = int(os.environ.get("CFG_PORT", "12348"))
    log_info(f"启动配置服务: http://{host}:{port}")
    uvicorn.run(app, host=host, port=port, log_level="info")