#!/usr/bin/env python3
import os, sys, json, base64, hashlib
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Random import get_random_bytes

ITERATIONS = 10
SALT_LEN   = 16
KEY_LEN    = 32
IV_LEN     = 16
LICENSE_PATH = "/root/unitree_2025_08_14/src/L2/src/ma/lisence"
KEY = "lxwxl"  # 与解密保持一致；生产环境不要硬编码

def encrypt_to_b64(plain_text: str, key: str) -> str:
    salt = get_random_bytes(SALT_LEN)
    iv = get_random_bytes(IV_LEN)
    key_iv = hashlib.pbkdf2_hmac('sha256', key.encode(), salt, ITERATIONS, KEY_LEN + IV_LEN)
    aes_key = key_iv[:KEY_LEN]
    cipher = AES.new(aes_key, AES.MODE_CBC, iv)
    ct = cipher.encrypt(pad(plain_text.encode(), AES.block_size))
    blob = salt + iv + ct
    return base64.b64encode(blob).decode()

def save_license_b64(b64_text: str, path: str = LICENSE_PATH):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(b64_text)

if __name__ == "__main__":
    # 你可以把需要加密的明文放在这里：
    # 示例1：直接对 NVMe 序列号明文进行加密
    # 示例2：对一个 JSON 许可对象进行加密
    mode = (sys.argv[1] if len(sys.argv) > 1 else "sn").lower()
    if mode == "sn":
        # 从已有文件或系统获取序列号（如果已保存为明文）
        src_path = LICENSE_PATH
        if not os.path.exists(src_path):
            print(f"❌ 未找到序列号文件 {src_path}，请先运行获取序列号脚本。")
            sys.exit(1)
        with open(src_path, "r", encoding="utf-8") as f:
            plain = f.read().strip()
    else:
        # 构造自定义许可信息
        lic_obj = {
            "sn": "REPLACE_WITH_NVME_SN",
            "expires": "2025-08-14",
            "features": ["face-recognition", "ros"],
        }
        plain = json.dumps(lic_obj, ensure_ascii=False)

    try:
        b64 = encrypt_to_b64(plain, KEY)
        save_license_b64(b64, LICENSE_PATH)
        print(f"✅ 已生成并保存加密许可证到: {LICENSE_PATH}")
    except Exception as e:
        print(f"❌ 加密失败: {e}")
        sys.exit(1)
