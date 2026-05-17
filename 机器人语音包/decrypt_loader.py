#!/usr/bin/env python3
import os
import marshal
import sys
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding

# Configuration (Must match encryption script)
ITERATIONS = 10000
KEY_LEN = 32
SALT_LEN = 16
IV_LEN = 16
# 内置固定密钥，无需用户输入
INTERNAL_PASSWORD = "Unitree_L2_Internal_Fixed_Key_2025_No_Input_Needed"

def run_encrypted_module(enc_file_path):
    """
    Decrypts and executes an encrypted Python module (.pyc.enc).
    """
    if not os.path.exists(enc_file_path):
        print(f"Error: File not found: {enc_file_path}")
        sys.exit(1)

    try:
        # 1. Read Encrypted File
        with open(enc_file_path, 'rb') as f:
            file_content = f.read()

        if len(file_content) < SALT_LEN + IV_LEN:
            print("Error: File is too short to be a valid encrypted file.")
            sys.exit(1)

        # 2. Extract Salt, IV, and Ciphertext
        salt = file_content[:SALT_LEN]
        iv = file_content[SALT_LEN:SALT_LEN+IV_LEN]
        ciphertext = file_content[SALT_LEN+IV_LEN:]

        # 3. Derive Key
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=KEY_LEN,
            salt=salt,
            iterations=ITERATIONS,
            backend=default_backend()
        )
        key = kdf.derive(INTERNAL_PASSWORD.encode())

        # 4. Decrypt
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        decryptor = cipher.decryptor()
        padded_data = decryptor.update(ciphertext) + decryptor.finalize()

        # 5. Unpad
        unpadder = padding.PKCS7(128).unpadder()
        marshaled_data = unpadder.update(padded_data) + unpadder.finalize()

        # 6. Load Code Object
        code_obj = marshal.loads(marshaled_data)

        # 7. Execute
        # Set up the global environment to mimic a standalone script
        module_globals = globals().copy()
        module_globals['__name__'] = '__main__'
        module_globals['__file__'] = enc_file_path
        
        exec(code_obj, module_globals)

    except ValueError as e:
        print("Decryption failed. Incorrect password or corrupted file.")
        # print(e) # Uncomment for debugging
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred during execution: {e}")
        sys.exit(1)

if __name__ == "__main__":
    # Example usage:
    # python3 decrypt_loader.py <path_to_enc_file>
    
    if len(sys.argv) < 2:
        print("Usage: python3 decrypt_loader.py <file.pyc.enc>")
        sys.exit(1)
        
    target_file = sys.argv[1]
    
    run_encrypted_module(target_file)
