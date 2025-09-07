#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QMap>
#include <QStandardItemModel>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include "NotesModel.h"
#include "../sync/SyncManager.h"

class QSplitter;
class QTreeView;
class QListView;
class QTextEdit;
class QToolBar;
class QAction;
class QComboBox;
class QPlainTextEdit;
class QTextBrowser;
class QLineEdit;
class TextEditor;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNoteSaved(int noteId);
    void onNoteDeleted(int noteId);
    void onFolderSaved(int folderId);
    void onFolderDeleted(int folderId);
    void onAutoSaveTriggered();
    void onTextChanged();
    void onAutoSaveTimeout();
    void showSettings();
    
    // Google Drive Sync slots
    void onGoogleDriveConnect();
    void onSyncNow();
    void onSyncSettings();
    void onSyncStatusChanged();
    void onSyncError(const QString &error);
    
    // Event handling
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupStyle();
    void createNewNote();
    void deleteSelectedNote();
    void createNewFolder();
    void deleteSelectedFolder();

    void setupContextMenus();
    void saveCurrentNote();
    void loadNoteContent(const QModelIndex &index);
    
    // Google Drive Sync setup
    void setupGoogleDriveSync();

    void onFolderSelected(const QModelIndex &index);
    void createNoteInCurrentFolder();
    void setupDatabaseConnections();
    void loadFoldersFromDatabase();
    void loadNotesFromDatabase(int folderId);
    void scheduleAutoSave();
    void importReadmeFiles();
    void manualImportMarkdownFiles();
    
    // Drag and drop handling
    void moveNoteToFolder(int noteId, int targetFolderId);
    bool canDropNoteOnFolder(int noteId, int targetFolderId);
    void restoreFolderSelection();

    QSplitter *m_mainSplitter;
    QTreeView *m_folderTree;
    QListView *m_noteList;
    // Text editor
    TextEditor *m_textEditor;

    QToolBar *m_toolbar;
    QAction *m_actNewNote;
    QAction *m_actDeleteNote;

    QAction *m_actNewFolder;
    QAction *m_actDeleteFolder;
    QAction *m_actSettings;
    
    // Google Drive Sync actions
    QAction *m_actConnectGoogleDrive;
    QAction *m_actSyncNow;
    QAction *m_actSyncSettings;

    
    // Note management
    QModelIndex m_currentNoteIndex;
    bool m_noteModified;
    int m_currentNoteId;
    
    // Folder management
    QModelIndex m_currentFolderIndex;
    int m_currentFolderId;
    QMap<QModelIndex, QStandardItemModel*> m_folderNotes;
    
    // Drag and drop state
    QModelIndex m_originalFolderSelection;
    
    // Auto-save functionality
    QTimer *m_autoSaveTimer;
    bool m_autoSaveEnabled;
    
    // Database models
    QStandardItemModel *m_folderModel;
    NotesModel *m_notesModel;
    
    // Google Drive Sync manager
    SyncManager *m_syncManager;
};


