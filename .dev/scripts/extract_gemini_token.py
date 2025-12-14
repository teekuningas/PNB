#!/usr/bin/env python3
"""
Extract OAuth access token from Gemini CLI storage for use in containers.

This script reads the encrypted token file and outputs just the access token.
Works similarly to `gh auth token` for GitHub Copilot.
"""

import os
import sys
import json
import hashlib
from pathlib import Path
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.scrypt import Scrypt
from cryptography.hazmat.backends import default_backend

GEMINI_DIR = '.gemini'
TOKEN_FILE = 'mcp-oauth-tokens-v2.json'
MAIN_ACCOUNT_KEY = 'main-account'


def get_token_path():
    """Get the path to the encrypted token file."""
    home = Path.home()
    return home / GEMINI_DIR / TOKEN_FILE


def derive_encryption_key():
    """Derive the encryption key using the same method as the Node.js code."""
    import socket
    import getpass
    
    hostname = socket.gethostname()
    username = getpass.getuser()
    salt_str = f"{hostname}-{username}-gemini-cli"
    
    # Use Scrypt with the same parameters as Node.js crypto.scryptSync
    kdf = Scrypt(
        salt=salt_str.encode('utf-8'),
        length=32,
        n=2**14,  # Default for Node.js scryptSync
        r=8,
        p=1,
        backend=default_backend()
    )
    key = kdf.derive(b'gemini-cli-oauth')
    return key


def decrypt(encrypted_data, key):
    """Decrypt the token data using AES-256-GCM."""
    parts = encrypted_data.split(':')
    if len(parts) != 3:
        raise ValueError('Invalid encrypted data format')
    
    iv = bytes.fromhex(parts[0])
    auth_tag = bytes.fromhex(parts[1])
    ciphertext = bytes.fromhex(parts[2])
    
    # Combine ciphertext and auth tag for AESGCM
    combined = ciphertext + auth_tag
    
    aesgcm = AESGCM(key)
    plaintext = aesgcm.decrypt(iv, combined, None)
    return plaintext.decode('utf-8')


def extract_token():
    """Extract the OAuth access token from storage (tries both old and new formats)."""
    # First, try the old unencrypted format
    old_token_path = Path.home() / GEMINI_DIR / 'oauth_creds.json'
    if old_token_path.exists():
        try:
            data = json.loads(old_token_path.read_text())
            if 'access_token' in data:
                print(data['access_token'])
                return
        except Exception as e:
            print(f"Warning: Failed to read old token format: {e}", file=sys.stderr)
    
    # Try the new encrypted format
    token_path = get_token_path()
    if not token_path.exists():
        print(f"Error: No token files found", file=sys.stderr)
        print(f"Tried: {old_token_path} and {token_path}", file=sys.stderr)
        print("Please run 'GEMINI_FORCE_FILE_STORAGE=true gemini' and use /login first.", file=sys.stderr)
        sys.exit(1)
    
    try:
        encrypted_data = token_path.read_text()
        key = derive_encryption_key()
        decrypted = decrypt(encrypted_data, key)
        tokens = json.loads(decrypted)
        
        account = tokens.get(MAIN_ACCOUNT_KEY)
        if not account or 'token' not in account or 'accessToken' not in account['token']:
            print("Error: No main account token found in encrypted file.", file=sys.stderr)
            print(f"Available keys: {list(tokens.keys())}", file=sys.stderr)
            sys.exit(1)
        
        # Output only the access token to stdout
        print(account['token']['accessToken'])
        
    except Exception as e:
        print(f"Error extracting token from encrypted file: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    extract_token()
