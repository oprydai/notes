# 🔒 Secure Google Drive Setup Guide

This guide explains how to set up Google Drive integration **securely** without exposing your API credentials in the open source code.

## 🚨 **Security First - Never Commit Credentials!**

Your Google API credentials are like passwords - **NEVER commit them to version control**. This project is designed to load credentials from secure sources that are not tracked by git.

## 📋 **Prerequisites**

- A Google account
- Access to Google Cloud Console
- Your Notes app source code

## 🔧 **Setup Steps**

### **Step 1: Create Google Cloud Project**

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing one
3. Enable Google Drive API:
   - Go to "APIs & Services" → "Library"
   - Search for "Google Drive API"
   - Click "Enable"

### **Step 2: Create OAuth 2.0 Credentials**

1. Go to "APIs & Services" → "Credentials"
2. Click "Create Credentials" → "OAuth 2.0 Client IDs"
3. Configure OAuth consent screen if prompted:
   - User Type: External
   - App name: "Notes App"
   - User support email: Your email
   - Developer contact information: Your email
4. Create OAuth 2.0 Client ID:
   - Application type: Desktop application
   - Name: "Notes App Desktop"
5. **Download the JSON credentials file**

### **Step 3: Choose Your Configuration Method**

You have **3 secure options** to configure your credentials:

#### **Option A: Environment Variables (Recommended for Development)**

1. Create a `.env` file in your project root:
```bash
cp env.example .env
```

2. Edit `.env` with your actual credentials:
```bash
GOOGLE_DRIVE_CLIENT_ID=your_actual_client_id
GOOGLE_DRIVE_CLIENT_SECRET=your_actual_client_secret
GOOGLE_DRIVE_SYNC_INTERVAL=15
GOOGLE_DRIVE_SYNC_FOLDER=Notes App
```

3. Source the environment variables:
```bash
source .env
```

4. **Important**: Add `.env` to your `.gitignore` (already done)

#### **Option B: Local Config File (Recommended for Production)**

1. Copy the example config:
```bash
cp config/google_drive_config.ini.example ~/.config/Notes/google_drive_config.ini
```

2. Edit the config file with your credentials:
```ini
client_id=your_actual_client_id
client_secret=your_actual_client_secret
sync_interval=15
sync_folder=Notes App
```

3. **Important**: This file is automatically ignored by git

#### **Option C: System Environment Variables (Most Secure)**

Set these in your shell profile (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
export GOOGLE_DRIVE_CLIENT_ID="your_actual_client_id"
export GOOGLE_DRIVE_CLIENT_SECRET="your_actual_client_secret"
export GOOGLE_DRIVE_SYNC_INTERVAL="15"
export GOOGLE_DRIVE_SYNC_FOLDER="Notes App"
```

Then reload your shell:
```bash
source ~/.bashrc  # or ~/.zshrc
```

## 🔍 **Verify Configuration**

The app will automatically validate your configuration when it starts. You can check if it's working by:

1. Building and running the app
2. Looking for these messages in the console:
   - ✅ "Configuration loaded from environment variables"
   - ✅ "Configuration loaded from config file"
   - ❌ "Configuration loaded from defaults (not functional)"

## 🛡️ **Security Features**

### **What's Protected:**
- ✅ API credentials are never in source code
- ✅ Configuration files are gitignored
- ✅ Environment variables are local only
- ✅ Tokens are stored securely in app data directory

### **What's Public:**
- ✅ OAuth flow implementation
- ✅ API integration code
- ✅ Sync logic and algorithms
- ✅ Example configuration templates

## 🚀 **Build and Test**

1. **Build the project:**
```bash
mkdir build && cd build
cmake ..
make
```

2. **Run the app:**
```bash
./Notes
```

3. **Test Google Drive integration:**
   - Go to Sync menu
   - Click "Connect to Google Drive"
   - Follow the OAuth flow

## 🔧 **Troubleshooting**

### **"Configuration not valid" Error**

Check your configuration:
```bash
# If using environment variables
echo $GOOGLE_DRIVE_CLIENT_ID
echo $GOOGLE_DRIVE_CLIENT_SECRET

# If using config file
cat ~/.config/Notes/google_drive_config.ini
```

### **"Client ID missing" Error**

- Ensure your credentials are properly set
- Check that the environment variables are loaded
- Verify the config file path and format

### **Build Errors**

- Ensure all source files are included in CMakeLists.txt
- Check that Qt Network module is available
- Verify the ConfigLoader is properly included

## 📁 **File Structure**

```
notes/
├── src/sync/
│   ├── ConfigLoader.h/cpp          # Secure credential loading
│   ├── GoogleDriveManager.h/cpp    # Google Drive API integration
│   ├── SyncManager.h/cpp           # Sync coordination
│   └── GoogleDriveConfig.h/cpp     # Configuration interface
├── config/
│   └── google_drive_config.ini.example  # Template config
├── env.example                     # Environment template
├── .gitignore                      # Protects sensitive files
└── SECURE_SETUP.md                 # This guide
```

## 🔄 **Updating Credentials**

If you need to update your credentials:

1. **Environment Variables**: Edit `.env` and reload
2. **Config File**: Edit `~/.config/Notes/google_drive_config.ini`
3. **System Variables**: Update shell profile and reload

## 🚨 **Security Checklist**

- [ ] Credentials are NOT in source code
- [ ] `.env` file is in `.gitignore`
- [ ] Config files are in `.gitignore`
- [ ] Environment variables are local only
- [ ] No credentials in commit history

## 🆘 **Need Help?**

If you encounter issues:

1. Check this guide first
2. Verify your configuration is valid
3. Check the console for error messages
4. Ensure all files are properly gitignored
5. Verify your Google Cloud Console setup

## 📜 **License & Security**

This integration follows security best practices:
- **OAuth 2.0** for secure authentication
- **No hardcoded credentials** in source code
- **Multiple secure configuration options**
- **Automatic validation** of configuration
- **Secure token storage** in app data directory

Your credentials are safe and secure when following this guide! 🔒✨
