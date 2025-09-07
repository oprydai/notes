#!/usr/bin/env python3
"""
Google Drive Configuration Validator
Run this script to verify your Google Drive API configuration before building the app.
"""

import os
import sys
from pathlib import Path

def check_environment_variables():
    """Check if environment variables are set."""
    print("🔍 Checking environment variables...")
    
    client_id = os.getenv('GOOGLE_DRIVE_CLIENT_ID')
    client_secret = os.getenv('GOOGLE_DRIVE_CLIENT_SECRET')
    
    if client_id and client_secret:
        print("✅ Environment variables found:")
        print(f"   Client ID: {client_id[:20]}...")
        print(f"   Client Secret: {client_secret[:10]}...")
        return True
    else:
        print("❌ Environment variables not found")
        if not client_id:
            print("   Missing: GOOGLE_DRIVE_CLIENT_ID")
        if not client_secret:
            print("   Missing: GOOGLE_DRIVE_CLIENT_SECRET")
        return False

def check_config_file():
    """Check if local config file exists and has credentials."""
    print("\n🔍 Checking local config file...")
    
    # Check multiple possible locations
    config_paths = [
        Path.home() / '.config' / 'Notes' / 'google_drive_config.ini',
        Path.cwd() / 'google_drive_config.ini',
        Path.cwd() / 'config' / 'google_drive_config.ini'
    ]
    
    for config_path in config_paths:
        if config_path.exists():
            print(f"✅ Config file found: {config_path}")
            
            try:
                with open(config_path, 'r') as f:
                    content = f.read()
                    
                if 'client_id=' in content and 'client_secret=' in content:
                    print("✅ Config file contains credentials")
                    return True
                else:
                    print("❌ Config file missing credentials")
                    return False
                    
            except Exception as e:
                print(f"❌ Error reading config file: {e}")
                return False
    
    print("❌ No config file found")
    return False

def check_gitignore():
    """Check if sensitive files are properly gitignored."""
    print("\n🔍 Checking .gitignore...")
    
    gitignore_path = Path.cwd() / '.gitignore'
    if not gitignore_path.exists():
        print("❌ .gitignore file not found")
        return False
    
    try:
        with open(gitignore_path, 'r') as f:
            content = f.read()
        
        sensitive_patterns = [
            '.env',
            'google_drive_config.ini',
            'google_drive_tokens.json',
            'sync_state.json'
        ]
        
        missing_patterns = []
        for pattern in sensitive_patterns:
            if pattern not in content:
                missing_patterns.append(pattern)
        
        if missing_patterns:
            print(f"❌ Missing from .gitignore: {', '.join(missing_patterns)}")
            return False
        else:
            print("✅ Sensitive files are properly gitignored")
            return True
            
    except Exception as e:
        print(f"❌ Error reading .gitignore: {e}")
        return False

def check_example_files():
    """Check if example configuration files exist."""
    print("\n🔍 Checking example files...")
    
    example_files = [
        'env.example',
        'config/google_drive_config.ini.example'
    ]
    
    missing_files = []
    for file_path in example_files:
        if not Path(file_path).exists():
            missing_files.append(file_path)
    
    if missing_files:
        print(f"❌ Missing example files: {', '.join(missing_files)}")
        return False
    else:
        print("✅ Example configuration files found")
        return True

def main():
    """Main validation function."""
    print("🔒 Google Drive Configuration Validator")
    print("=" * 50)
    
    checks = [
        check_environment_variables,
        check_config_file,
        check_gitignore,
        check_example_files
    ]
    
    results = []
    for check in checks:
        try:
            results.append(check())
        except Exception as e:
            print(f"❌ Error during check: {e}")
            results.append(False)
    
    print("\n" + "=" * 50)
    print("📊 Validation Results:")
    
    passed = sum(results)
    total = len(results)
    
    if passed == total:
        print("🎉 All checks passed! Your configuration is secure.")
        print("\n✅ You can now build and run the app.")
        print("✅ Your credentials are protected from accidental commits.")
        return 0
    else:
        print(f"⚠️  {passed}/{total} checks passed")
        print("\n❌ Please fix the issues above before proceeding.")
        print("❌ Do not build the app until all security issues are resolved.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
