# Notes (Qt, C++17, SQLite)

A fast, native Linux note-taking application built with C++ and Qt. This repository features a 3-pane layout using `QSplitter`, with SQLite persistence, auto-save functionality, and a clean plain text editor.

## 🎉 **Status: Successfully Installed & Ready to Use!**

The Notes application has been successfully built and installed as a system package on Ubuntu. You can now:

- **Launch**: Search for "Notes" in your applications menu or run `notes` from terminal
- **Features**: All requested features implemented including drag & drop, auto-save, and Obsidian-like editor
- **Package**: Professional .deb package with proper system integration
- **Documentation**: Complete build and usage instructions included below

## Features

### ✨ Core Features
- **3-Pane Layout**: Folders, notes list, and plain text editor
- **Persistent Storage**: SQLite database with automatic data persistence
- **Auto-Save**: Automatic saving of notes with configurable intervals
- **File System Integration**: Import existing text files and export notes
- **Configurable Storage**: User-selectable notes directory
- **Enhanced Drag & Drop**: Move notes between folders with visual feedback

### 📝 Editor Features
- **Plain Text Editor**: Simple, clean text editing
- **Auto-Save**: Automatic saving with configurable intervals
- **Basic Keyboard Shortcuts**:
  - `Ctrl+N`: New note
  - `Ctrl+S`: Save (manual save)
  - `Delete`: Delete selected note/folder
  - `Ctrl+P`: Pin/unpin note
  - `Ctrl+D`: Duplicate line
  - `Ctrl+L`: Select current line
  - `Ctrl+Home/End`: Start/End of line
  - `Alt+Up/Down`: Move line up/down

### 🗂️ Organization Features
- **Folder Management**: Create, rename, delete folders
- **Note Management**: Create, edit, delete, pin notes
- **Search**: Real-time search through notes
- **Pinning**: Pin important notes to the top
- **Import/Export**: Import text files and export notes
- **Drag & Drop**: Move notes between folders with visual feedback
  - **Visual Feedback**: Folders highlight when dragging notes over them
  - **Real-time Preview**: See exactly which folder you're targeting
  - **Selection Preservation**: Original folder selection is maintained

### 🎨 User Interface
- **Modern Dark Theme**: Apple Notes-inspired design
- **Reordered Toolbar**: Logical layout with folder actions first
  - **Leftmost**: New Folder, Delete Folder
  - **Center**: New Note, Delete, Pin/Unpin
  - **Right**: Settings, Search, Preview Toggle
- **Clean Note Display**: Notes list shows only title and date
- **White Title Bar**: Window title and control buttons in white
- **Persistent Icons**: Folder icons remain after deletion operations

### ⚙️ Settings & Configuration
- **Auto-Save Settings**: Enable/disable and configure intervals
- **Notes Directory**: Choose where notes are stored
- **Dark Theme**: Modern dark UI with Apple Notes-inspired design

## Build Instructions

Prerequisites:
- Qt 6 (Widgets, Sql) or Qt 5.15+
- CMake 3.16+
- A C++17 compiler (GCC 9+/Clang 10+)

On Ubuntu/Debian (example):
```bash
sudo apt update
sudo apt install -y cmake g++ qtbase5-dev qtbase5-dev-tools libqt5sql5 sqlite3
# For Qt6 instead of Qt5:
# sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

Configure and build:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/Notes
```

### Building a .deb Package (Recommended for Installation)

For easy system-wide installation, you can build a Debian package:

#### Prerequisites for .deb building:
```bash
sudo apt update
sudo apt install -y build-essential devscripts debhelper dh-make
```

#### Build the .deb package:
```bash
# From the project root directory
dpkg-buildpackage -b -us -uc
```

#### Install the .deb package:
```bash
# The .deb file will be created in the parent directory
sudo dpkg -i ../notes_0.1.0-1_amd64.deb

# If there are dependency issues, run:
sudo apt-get install -f
```

#### Alternative: Build and install in one command:
```bash
# Build and install directly
sudo dpkg-buildpackage -b -us -uc && sudo dpkg -i ../notes_0.1.0-1_amd64.deb
```

After installation, you can:
- **Launch from System Menu**: Search for "Notes" in your applications menu
- **Run from Terminal**: Type `notes` from any directory
- **Desktop Shortcut**: Right-click desktop → Create Launcher → Application: `notes`
- **Uninstall**: `sudo apt remove notes` (removes all files cleanly)
- **Update**: `sudo apt update && sudo apt upgrade notes` (when new versions are available)

#### What the .deb package includes:
- **Binary**: `/usr/bin/notes` - The main application executable
- **Desktop Entry**: `/usr/share/applications/notes.desktop` - Application launcher
- **Icons**: Multiple resolution icons in `/usr/share/icons/hicolor/`
- **Dependencies**: Automatically handles Qt and SQLite dependencies

#### Package Information:
- **Package Name**: `notes`
- **Version**: `0.1.0-1`
- **Architecture**: `amd64`
- **Section**: `utils`
- **Priority**: `optional`
- **Status**: ✅ **Successfully Installed** (as of latest build)

#### Installation Verification:
```bash
# Check if installed
dpkg -l | grep notes

# Check executable location
which notes

# List installed files
dpkg -L notes

# Test launch
notes
```

#### Troubleshooting .deb Build Issues:

**Error: "dpkg-buildpackage: command not found"**
```bash
sudo apt install -y build-essential devscripts debhelper dh-make
```

**Error: "Cannot find debian/rules"**
```bash
# Make sure you're in the project root directory
ls debian/rules
```

**Error: "Build dependencies not satisfied"**
```bash
# Install build dependencies
sudo apt build-dep .
```

**Error: "Package has unmet dependencies"**
```bash
# After installing the .deb, fix dependencies
sudo apt-get install -f
```

**Clean build environment:**
```bash
# Remove previous build artifacts
dpkg-buildpackage -b -us -uc -tc
```

## Usage

### First Launch
1. The application will create default folders (Personal, Work, Ideas, etc.)
2. Click the Settings button (⚙️) to configure your notes directory
3. Choose a directory where you want to store your notes
4. Enable auto-save and set your preferred interval

### Creating Notes
1. Select a folder from the left panel
2. Click "New Note" or press `Ctrl+N`
3. Start typing - notes auto-save as you type
4. Simple plain text editing - no special formatting needed

### Moving Notes Between Folders
1. **Drag & Drop Method**:
   - Click and drag a note from the notes list
   - Drag it over a folder in the folder tree
   - The folder will highlight as you hover over it
   - Drop the note to move it to that folder
   - Status bar confirms the move

2. **Visual Feedback**:
   - Folders light up in real-time as you drag over them
   - Clear indication of which folder is the drop target
   - Original folder selection is preserved after the move

### Importing Existing Files
1. Place your text files in the configured notes directory
2. Click "Import Text Files" in settings
3. Files will be imported into an "Imported" folder

### Keyboard Shortcuts
- `Ctrl+N`: New note
- `Ctrl+S`: Save (manual save)
- `Delete`: Delete selected note/folder
- `Ctrl+P`: Pin/unpin note
- `Ctrl+F`: Focus search
- `Ctrl+Shift+N`: New folder

## Architecture

### Database Schema
- **folders**: Folder hierarchy with parent-child relationships
- **notes**: Note content with metadata (title, body, timestamps, pinned status)
- **tags**: Tag system for note organization
- **note_tags**: Many-to-many relationship between notes and tags
- **attachments**: File attachments support
- **settings**: Application settings storage

### Auto-Save Mechanism
- Notes are automatically saved after 2 seconds of inactivity
- Immediate save when switching between notes or folders
- Save on application focus loss
- Configurable save intervals (1-60 seconds)

### Drag & Drop Implementation
- **Custom NotesModel**: Handles drag data with note IDs
- **Event Filter**: Intercepts drop events on folder tree
- **Visual Feedback**: Real-time folder highlighting during drag
- **Database Integration**: Updates note folder relationships
- **Selection Management**: Preserves original folder selection

### File System Integration
- Notes can be exported as individual text files
- Text files are automatically imported
- Configurable notes directory with automatic creation
- File watching for external changes (planned)

## Project Structure
```
src/
├── db/
│   ├── DatabaseManager.h/cpp    # Database operations and persistence
├── ui/
│   ├── MainWindow.h/cpp         # Main application window
│   ├── TextEditor.h/cpp         # Plain text editor
│   ├── SettingsDialog.h/cpp     # Settings configuration
│   ├── NoteListDelegate.h/cpp   # Custom note list display
│   └── NotesModel.h/cpp         # Custom model for drag & drop
├── utils/
│   └── Roles.h                  # Qt model roles
└── main.cpp                     # Application entry point
```

## Data Storage

### Database Location
- **Linux**: `~/.local/share/Notes/notes.db`
- **Settings**: `~/.local/share/Notes/settings.ini`

### Notes Directory
- **Default**: `~/Documents/Notes/`
- **Configurable**: User can choose any directory
- **Auto-creation**: Directory is created if it doesn't exist

## Recent Updates

### ✅ **Successfully Installed as System Package** (Latest)
- **System Integration**: Notes application is now installed system-wide via .deb package
- **Desktop Integration**: Appears in system applications menu with proper icon
- **Terminal Access**: Can be launched with `notes` command from anywhere
- **Professional Packaging**: Follows Debian packaging standards
- **Easy Updates**: Can be updated through package manager
- **Clean Uninstall**: `sudo apt remove notes` removes all files

### Enhanced Drag & Drop
- ✅ **Visual Feedback**: Folders highlight when dragging notes over them
- ✅ **Real-time Updates**: Immediate visual response during drag operations
- ✅ **Selection Preservation**: Original folder selection is maintained
- ✅ **Smooth Interactions**: No jarring visual changes during drag
- ✅ **Status Confirmation**: Clear feedback when notes are moved

### Improved Toolbar Layout
- ✅ **Logical Ordering**: Folder actions (New Folder, Delete Folder) moved to leftmost position
- ✅ **Better Workflow**: Container actions before content actions
- ✅ **Visual Separation**: Clear separators between action groups
- ✅ **Intuitive Layout**: Follows natural note-taking workflow

### UI Polish
- ✅ **Clean Note Display**: Notes list shows only title and date (no snippet text)
- ✅ **White Title Bar**: Window title and control buttons styled in white
- ✅ **Persistent Icons**: Folder icons remain after deletion operations
- ✅ **Enhanced CSS**: Better visual states for drag and drop interactions

## Future Enhancements
- [ ] Full-text search with highlighting
- [ ] Tag system implementation
- [ ] File attachments support
- [ ] Note templates
- [ ] Export to various formats (PDF, HTML)
- [ ] Cloud sync integration
- [ ] Collaborative editing
- [ ] Version history
- [ ] Note encryption
- [ ] Mobile companion app
- [ ] Advanced drag & drop (multiple selection, keyboard modifiers)
- [ ] Folder color coding
- [ ] Note sorting options (date, title, size)

## License
MIT
