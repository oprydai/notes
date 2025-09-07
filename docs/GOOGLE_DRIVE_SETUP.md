# Google Drive Integration Setup Guide

This guide will walk you through setting up Google Drive integration for your Notes app.

## Prerequisites

- A Google account
- Access to Google Cloud Console
- Your Notes app source code

## Step 1: Create a Google Cloud Project

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Click "Select a project" → "New Project"
3. Enter a project name (e.g., "Notes App")
4. Click "Create"

## Step 2: Enable Google Drive API

1. In your project, go to "APIs & Services" → "Library"
2. Search for "Google Drive API"
3. Click on "Google Drive API"
4. Click "Enable"

## Step 3: Create OAuth 2.0 Credentials

1. Go to "APIs & Services" → "Credentials"
2. Click "Create Credentials" → "OAuth 2.0 Client IDs"
3. If prompted, configure the OAuth consent screen:
   - User Type: External
   - App name: "Notes App"
   - User support email: Your email
   - Developer contact information: Your email
   - Save and continue through the remaining steps

4. Back in "Create OAuth 2.0 Client IDs":
   - Application type: Desktop application
   - Name: "Notes App Desktop"
   - Click "Create"

5. **Important**: Download the JSON credentials file

## Step 4: Update Configuration

1. Open `src/sync/GoogleDriveConfig.cpp`
2. Replace the placeholder values:

```cpp
const QString GoogleDriveConfig::CLIENT_ID = "YOUR_ACTUAL_CLIENT_ID";
const QString GoogleDriveConfig::CLIENT_SECRET = "YOUR_ACTUAL_CLIENT_SECRET";
```

3. Also update `src/ui/GoogleAuthDialog.cpp`:

```cpp
QString clientId = "YOUR_ACTUAL_CLIENT_ID"; // Replace this line
```

## Step 5: Build and Test

1. Build your project:
```bash
mkdir build && cd build
cmake ..
make
```

2. Run the app and test Google Drive integration

## How It Works

### Authentication Flow
1. User clicks "Connect to Google Drive"
2. App opens browser to Google OAuth page
3. User signs in and grants permissions
4. Google shows authorization code
5. User copies code and pastes it in the app
6. App exchanges code for access/refresh tokens
7. Tokens are saved locally for future use

### Sync Process
1. **Auto-sync**: Runs every 15 minutes (configurable)
2. **Manual sync**: User can trigger sync anytime
3. **Conflict resolution**: Automatically resolves conflicts (can be enhanced)
4. **File storage**: Notes are stored as `.md` files in a dedicated Google Drive folder

### File Structure
- Notes are stored as Markdown files (`.md`)
- Each note gets a unique ID mapping between local and remote
- Sync state is saved locally in JSON format
- Tokens are stored securely in app data directory

## Security Features

- **OAuth 2.0**: Secure authentication without sharing passwords
- **Limited scope**: Only accesses files created by the app (`drive.file`)
- **Token refresh**: Automatically refreshes expired tokens
- **Local storage**: Credentials stored securely on user's machine

## Troubleshooting

### Common Issues

1. **"Invalid client" error**
   - Check that CLIENT_ID and CLIENT_SECRET are correct
   - Ensure OAuth consent screen is configured

2. **"Redirect URI mismatch"**
   - Desktop apps use `urn:ietf:wg:oauth:2.0:oob`
   - This is already configured in the code

3. **API not enabled**
   - Make sure Google Drive API is enabled in your project

4. **Build errors**
   - Ensure Qt Network module is available
   - Check that all source files are included in CMakeLists.txt

### Debug Mode

To see detailed API requests/responses, add this to your code:

```cpp
#include <QDebug>
// In GoogleDriveManager methods:
qDebug() << "API Request:" << url;
qDebug() << "API Response:" << response;
```

## Next Steps

### Enhancements to Consider

1. **Better conflict resolution**: Show diff view for conflicting notes
2. **Selective sync**: Let users choose which notes to sync
3. **Offline support**: Queue changes when offline
4. **Multiple accounts**: Support for multiple Google accounts
5. **Backup**: Automatic backup before sync operations

### Production Considerations

1. **Rate limiting**: Google Drive API has quotas
2. **Error handling**: Implement retry logic for failed operations
3. **User feedback**: Show sync progress and status
4. **Testing**: Test with various network conditions

## API Quotas

- **Daily quota**: 1,000,000 requests per day
- **Per user per 100 seconds**: 1,000 requests
- **Per user per 100 seconds per user**: 10,000 requests

For most users, these limits are more than sufficient.

## Support

If you encounter issues:
1. Check the troubleshooting section above
2. Verify your Google Cloud Console settings
3. Check the app logs for error messages
4. Ensure you're using the latest version of the code

## License

This integration follows Google's OAuth 2.0 best practices and is designed for personal use. For commercial applications, review Google's terms of service and API usage policies.
