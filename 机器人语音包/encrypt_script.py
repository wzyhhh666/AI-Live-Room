#!/usr/bin/env python3
import os
import marshal
import argparse
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding

# Configuration
ITERATIONS = 10000
KEY_LEN = 32  # AES-256
SALT_LEN = 16
IV_LEN = 16
# 内置固定密钥，无需用户输入
INTERNAL_PASSWORD = "Unitree_L2_Internal_Fixed_Key_2025_No_Input_Needed"

def encrypt_script(source_file, output_file):
    """
    Compiles a Python script to a code object, marshals it, and encrypts it.
    """
    print(f"Processing: {source_file}")
    
    # 1. Read and Compile Source Code
    try:
        with open(source_file, 'r', encoding='utf-8') as f:
            source_code = f.read()
        code_obj = compile(source_code, source_file, 'exec')
    except Exception as e:
        print(f"Error compiling {source_file}: {e}")
        return

    # 2. Marshal the code object (serialize)
    data = marshal.dumps(code_obj)

    # 3. Prepare Encryption Parameters
    salt = os.urandom(SALT_LEN)
    iv = os.urandom(IV_LEN)

    # 4. Derive Key using PBKDF2
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=KEY_LEN,
        salt=salt,
        iterations=ITERATIONS,
        backend=default_backend()
    )
    key = kdf.derive(INTERNAL_PASSWORD.encode())

    # 5. Encrypt Data (AES-CBC with PKCS7 Padding)
    padder = padding.PKCS7(128).padder()
    padded_data = padder.update(data) + padder.finalize()

    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(padded_data) + encryptor.finalize()

    # 6. Write Output File (Salt + IV + Ciphertext)
    try:
        with open(output_file, 'wb') as f:
            f.write(salt)
            f.write(iv)
            f.write(ciphertext)
        print(f"Success! Encrypted file saved to: {output_file}")
    except Exception as e:
        print(f"Error writing output file: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Encrypt a Python script.")
    parser.add_argument("source", help="Path to the source .py file")
    parser.add_argument("output", help="Path to the output .pyc.enc file")
    
    args = parser.parse_args()
    
    encrypt_script(args.source, args.output)
