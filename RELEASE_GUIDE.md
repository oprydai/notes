# GitHub Release Guide

## 🚨 The Problem

The download link `https://github.com/mustafa-khann/notes/releases/latest/download/notes_0.1.0-1_amd64.deb` returns a 404 error because:

1. **No GitHub release exists yet** - You need to create a release first
2. **The .deb file isn't uploaded** - The file needs to be attached to the release
3. **Wrong URL format** - GitHub releases use specific version tags, not "latest"

## ✅ Solution: Create a GitHub Release

### Option 1: Using the Automated Script (Recommended)

I've created a script to automate the release process:

```bash
# Run the release creation script
./create-release.sh
```

**Prerequisites:**
1. Install GitHub CLI: `sudo apt install gh`
2. Authenticate: `gh auth login`
3. Make sure you have the .deb file: `notes_0.1.0-1_amd64.deb`

### Option 2: Manual Web Interface

1. **Go to your repository**: https://github.com/mustafa-khann/notes
2. **Click "Releases"** (right sidebar)
3. **Click "Create a new release"**
4. **Fill in details**:
   - **Tag version**: `v0.1.0`
   - **Release title**: `Notes v0.1.0 - Initial Release`
   - **Description**: Copy from the script output
5. **Upload the .deb file**: Drag `notes_0.1.0-1_amd64.deb` to "Attach binaries"
6. **Click "Publish release"**

## 🔗 Correct Download Links

After creating the release, use these URLs:

### Specific Version (Recommended)
```bash
wget https://github.com/mustafa-khann/notes/releases/download/v0.1.0/notes_0.1.0-1_amd64.deb
```

### Latest Version (After Release)
```bash
wget https://github.com/mustafa-khann/notes/releases/latest/download/notes_0.1.0-1_amd64.deb
```

**Note**: The "latest" link only works after you've created at least one release.

## 📋 Step-by-Step Instructions

### Step 1: Install GitHub CLI
```bash
# Add GitHub CLI repository
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null

# Install GitHub CLI
sudo apt update
sudo apt install gh
```

### Step 2: Authenticate with GitHub
```bash
gh auth login
# Follow the prompts to authenticate
```

### Step 3: Create the Release
```bash
# Make sure you're in the main branch
git checkout main

# Run the release script
./create-release.sh
```

### Step 4: Verify the Release
1. Go to: https://github.com/mustafa-khann/notes/releases
2. You should see "v0.1.0" release
3. The .deb file should be downloadable

### Step 5: Test the Download
```bash
# Test the download link
wget https://github.com/mustafa-khann/notes/releases/download/v0.1.0/notes_0.1.0-1_amd64.deb
```

## 🎯 What the Script Does

The `create-release.sh` script:

1. ✅ **Checks prerequisites**: .deb file exists, GitHub CLI installed, authenticated
2. ✅ **Creates release description**: Professional markdown with features and installation instructions
3. ✅ **Creates GitHub release**: With proper tag, title, and description
4. ✅ **Uploads .deb file**: Attaches the binary to the release
5. ✅ **Provides feedback**: Shows you the download links

## 🔄 Updating for Future Releases

For future versions (v0.1.1, v0.2.0, etc.):

1. **Update version in script**:
   ```bash
   # Edit create-release.sh
   VERSION="v0.1.1"
   TAG="v0.1.1"
   RELEASE_TITLE="Notes v0.1.1 - Bug Fixes"
   DEB_FILE="notes_0.1.1-1_amd64.deb"
   ```

2. **Build new .deb package**:
   ```bash
   dpkg-buildpackage -b -us -uc
   ```

3. **Create new release**:
   ```bash
   ./create-release.sh
   ```

4. **Update landing page** (if needed):
   ```bash
   ./update-landing-page.sh
   ```

## 🐛 Troubleshooting

### "gh: command not found"
```bash
sudo apt install gh
```

### "Not authenticated with GitHub"
```bash
gh auth login
```

### ".deb file not found"
```bash
# Build the package first
dpkg-buildpackage -b -us -uc
```

### "Release already exists"
```bash
# Delete existing release first
gh release delete v0.1.0
# Then create new one
./create-release.sh
```

### "Permission denied"
```bash
# Make script executable
chmod +x create-release.sh
```

## 📊 Release URLs

After creating the release, these URLs will work:

- **Release page**: https://github.com/mustafa-khann/notes/releases/tag/v0.1.0
- **Download link**: https://github.com/mustafa-khann/notes/releases/download/v0.1.0/notes_0.1.0-1_amd64.deb
- **Latest download**: https://github.com/mustafa-khann/notes/releases/latest/download/notes_0.1.0-1_amd64.deb

## 🎉 Success Checklist

- [ ] GitHub CLI installed and authenticated
- [ ] .deb package built successfully
- [ ] GitHub release created with v0.1.0 tag
- [ ] .deb file uploaded to release
- [ ] Download link works: `wget https://github.com/mustafa-khann/notes/releases/download/v0.1.0/notes_0.1.0-1_amd64.deb`
- [ ] Landing page updated with correct download link
- [ ] GitHub Pages enabled and working

---

**Once you complete these steps, your download links will work perfectly! 🚀**
