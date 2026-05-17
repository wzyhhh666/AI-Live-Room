#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rospy + FastAPI 无类示例
POST /frame  接收设备搜索请求
"""

import rospy
from fastapi import FastAPI, Request, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
import threading
import base64
import json
from typing import Dict, Any

# ---------- ROS 初始化 ----------
rospy.init_node('search_hook', anonymous=True)

# ---------- FastAPI 应用 ----------
app = FastAPI(title="rospy-search", version="0.1.0")

# 添加 CORS 中间件
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 生产环境应限制为具体域名
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

def decrypt_id(encoded: str) -> Dict[str, Any]:
    """
    对应前端 encryptId 的逆向操作。
    """
    try:
        # 1. Base64 解码
        combined_bytes = base64.b64decode(encoded, validate=True)
        combined = combined_bytes.decode('utf-8')

        # 2. 按 ":" 拆分
        salt, ori_id, ts36 = combined.split(':', 2)

        # 3. 36 进制转 10 进制
        ts10 = int(ts36, 36)

        return {
            'id': ori_id,
            'salt': salt,
            'timestamp36': ts36,
            'timestamp10': ts10
        }
    except Exception as e:
        raise ValueError(f"解密失败: {e}") from e

@app.get("/")
async def root():
    """健康检查端点"""
    return {"status": "running", "service": "search_hook"}

@app.get("/frame")
async def get_frame():
    """处理 GET 请求，返回明确错误信息"""
    return {"error": "Method not allowed", "message": "请使用 POST 方法"}

@app.post("/frame")
async def hook_handler(request: Request):
    """
    接收设备搜索请求
    """
    try:
        # 记录请求信息
        client_host = request.client.host
        rospy.loginfo(f"收到来自 {client_host} 的 POST 请求")
        
        # 解析请求体
        payload = await request.json()
        rospy.loginfo(f"请求数据: {payload}")
        
        # 检查必要字段
        if "id" not in payload:
            rospy.logwarn("请求中缺少 'id' 字段")
            return {"ok": False, "error": "Missing 'id' field"}
        
        # 解密 ID
        decrypted = decrypt_id(payload["id"])
        rospy.loginfo(f"解密结果: {decrypted}")
        
        # 验证 ID
        if decrypted["id"] == "xiaoxi":
            rospy.loginfo("ID 验证成功: xiaoxi")
            return {"ok": True, "message": "设备连接成功"}
        else:
            rospy.logwarn(f"ID 验证失败: {decrypted['id']}")
            return {"ok": False, "error": "Invalid device ID"}
            
    except json.JSONDecodeError as e:
        rospy.logerr(f"JSON 解析错误: {e}")
        return {"ok": False, "error": "Invalid JSON format"}
    except ValueError as e:
        rospy.logerr(f"解密错误: {e}")
        return {"ok": False, "error": str(e)}
    except Exception as e:
        rospy.logerr(f"处理请求时发生错误: {e}")
        return {"ok": False, "error": "Internal server error"}

# ---------- 启动 HTTP 服务器 ----------
def run_fastapi():
    try:
        rospy.loginfo("启动 FastAPI 服务器在 0.0.0.0:12349")
        uvicorn.run(
            app, 
            host="0.0.0.0", 
            port=12349, 
            log_level="info",
            access_log=True
        )
    except Exception as e:
        rospy.logerr(f"FastAPI 服务器启动失败: {e}")

if __name__ == "__main__":
    # 把 FastAPI 跑在子线程，避免阻塞 rospy 主线程
    server_thread = threading.Thread(target=run_fastapi, daemon=True)
    server_thread.start()
    
    rospy.loginfo("Search service 启动完成")
    rospy.loginfo("HTTP 服务监听: http://0.0.0.0:12349")
    rospy.loginfo("设备搜索端点: POST http://0.0.0.0:12349/frame")
    
    try:
        rospy.spin()
    except KeyboardInterrupt:
        rospy.loginfo("程序被用户中断")