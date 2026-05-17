#!/usr/bin/env python3
import base64, hashlib, sys
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

ITERATIONS = 10
SALT_LEN   = 16
KEY_LEN    = 32
IV_LEN     = 16
LICENSE_PATH = "/root/unitree_2025_08_14/src/jiami/lisence"
KEY = "lxwxl"

def decrypt(b64_cipher: str, key: str) -> str:
    raw = base64.b64decode(b64_cipher.encode())
    salt, iv, ct = raw[:SALT_LEN], raw[SALT_LEN:SALT_LEN+IV_LEN], raw[SALT_LEN+IV_LEN:]
    key_iv = hashlib.pbkdf2_hmac('sha256', key.encode(), salt, ITERATIONS, KEY_LEN + IV_LEN)
    aes_key = key_iv[:KEY_LEN]
    cipher = AES.new(aes_key, AES.MODE_CBC, iv)
    return unpad(cipher.decrypt(ct), AES.block_size).decode()

def read_license(path: str = LICENSE_PATH) -> str:
    try:
        with open(path, "r", encoding="utf-8") as f:
            return f.read().strip()
    except FileNotFoundError:
        raise SystemExit(f"[ERR] 找不到许可证文件：{path}")
    except OSError as e:
        raise SystemExit(f"[ERR] 读取许可证失败：{e}")

if __name__ == "__main__":
    try:
        cipher_text = read_license()
        plain = decrypt(cipher_text, KEY)
        print("✅ 解密成功，许可证明文如下：")
        print(plain)
    except Exception as e:
        print(f"[ERR] 解密失败：{e}", file=sys.stderr)
        sys.exit(1)
