#!/usr/bin/env python3
import subprocess, shutil, sys, os

DEV = "/dev/nvme0n1"
LICENSE_PATH = "/root/unitree_2025_08_14/src/L2/src/ma/lisence"   # 许可证文件路径

def get_nvme_sn(dev: str = DEV) -> str:
    """返回 NVMe 固件序列号（纯字符串）"""
    if not shutil.which("nvme"):
        raise SystemExit("nvme-cli 未安装，运行:  sudo apt install nvme-cli")

    try:
        out = subprocess.check_output(["sudo", "nvme", "id-ctrl", dev],
                                      stderr=subprocess.STDOUT, text=True)
    except subprocess.CalledProcessError as e:
        raise SystemExit(f"nvme 命令失败: {e.output.strip()}")

    for line in out.splitlines():
        if line.strip().startswith("sn"):
            sn = line.split(":", 1)[1].strip()
            return sn
    raise ValueError("未找到序列号字段")

def save_sn(sn: str, path: str = LICENSE_PATH) -> None:
    """把序列号写入指定文件（自动创建目录）"""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(sn)

if __name__ == "__main__":
    try:
        sn = get_nvme_sn()
        save_sn(sn)
        print(f"✅ 已保存 NVMe 序列号到: {LICENSE_PATH}")
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(1)

