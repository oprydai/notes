# Markdown Storage System

This document describes the implementation of the new Markdown storage system for the Notes application.

## Overview

The application now stores notes in two formats:
1. **SQLite Database**: Stores metadata, organization, and search information
2. **Markdown Files**: Stores the actual note content in `.md` files with frontmatter

## Key Features

### 1. Automatic Markdown File Creation
- Every note is automatically saved as a `.md` file
- Files are stored in the configured notes directory (default: `~/Documents/Notes`)
- Filenames are generated from note titles with timestamps for uniqueness

### 2. Frontmatter Support
Each markdown file includes YAML frontmatter with:
```yaml
---
title: "Note Title"
created: 2024-01-15T10:30:00Z
modified: 2024-01-15T15:45:00Z
folder_id: 1
---
```

### 3. Database Schema Updates
The `notes` table now includes:
- `filepath`: Path to the corresponding `.md` file
- All existing fields remain unchanged

### 4. Auto-save Integration
- Notes are automatically saved to markdown files when created/updated
- Auto-save timer triggers markdown file updates
- File system and database stay in sync

## Implementation Details

### New Methods Added

#### Markdown File Operations
- `generateMarkdownFilename()`: Creates safe filenames from titles
- `saveNoteToMarkdownFile()`: Saves note content to `.md` file
- `loadNoteFromMarkdownFile()`: Loads note content from `.md` file
- `getNoteFilePath()`: Returns full path to markdown file
- `ensureNoteFileExists()`: Creates file if missing
- `validateMarkdownFile()`: Validates file integrity
- `getMarkdownFileList()`: Lists all `.md` files
- `syncNoteWithFile()`: Syncs database with file system

#### Bulk Operations
- `syncAllNotesWithFiles()`: Syncs all notes with their files
- `recreateAllMarkdownFiles()`: Recreates all markdown files
- `scanAndImportMarkdownFiles()`: Imports existing `.md` files

#### Auto-save Enhancement
- `markNoteAsModified()`: Tracks modified notes for auto-save
- Enhanced `performAutoSave()`: Saves to markdown files

### Database Migration
- Automatically adds `filepath` column to existing databases
- Converts existing notes to markdown format
- Maintains backward compatibility

## File Structure

```
~/Documents/Notes/
├── Meeting_Notes_20240115_143022.md
├── Project_Ideas_20240115_143045.md
├── Personal_Tasks_20240115_143100.md
└── ...
```

## Usage Examples

### Creating a Note
```cpp
DatabaseManager &db = DatabaseManager::instance();
int noteId = db.createNote(folderId, "Title", "# Content\n\nMarkdown content here");
// Automatically creates: Title_20240115_143022.md
```

### Updating a Note
```cpp
db.updateNote(noteId, "New Title", "# New Content\n\nUpdated markdown");
// Automatically updates the .md file
```

### Getting File Path
```cpp
QString filePath = db.getNoteFilePath(noteId);
// Returns: ~/Documents/Notes/Title_20240115_143022.md
```

### Syncing with Files
```cpp
// Sync all notes with their files
db.syncAllNotesWithFiles();

// Sync specific note
db.syncNoteWithFile(noteId);
```

## Benefits

1. **Human-readable**: Notes can be edited with any text editor
2. **Version Control**: Git-friendly format for tracking changes
3. **Portability**: Notes can be moved between systems
4. **Future-proof**: Markdown is a stable, open format
5. **Rich Formatting**: Supports headers, lists, code blocks, etc.
6. **Searchable**: Text-based format works with standard search tools

## Migration from Previous System

The system automatically handles migration:
1. Existing notes are converted to markdown format
2. Database schema is updated automatically
3. No data loss occurs
4. All existing functionality remains intact

## Testing

Use the provided test file `test_markdown_system.cpp` to verify functionality:

```bash
# Compile and run the test
g++ -std=c++17 -I. test_markdown_system.cpp -o test_markdown
./test_markdown
```

## Configuration

The notes directory can be configured:
```cpp
db.setNotesDirectory("/path/to/notes");
```

Default location: `~/Documents/Notes`

## Error Handling

- File I/O errors are logged with `qWarning()`
- Database operations continue even if file operations fail
- Automatic file recreation on missing files
- Validation methods ensure file integrity

## Future Enhancements

Potential improvements:
- Support for additional frontmatter fields (tags, categories)
- Markdown syntax highlighting in the editor
- Export to other formats (HTML, PDF)
- Integration with external markdown editors
- Backup and restore functionality
