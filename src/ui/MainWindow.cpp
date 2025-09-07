#include "MainWindow.h"

#include <QApplication>
#include <QSplitter>
#include <QFile>
#include <QTreeView>
#include <QListView>
#include <QTextEdit>
#include <QToolBar>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QIcon>
#include <QLabel>
#include <QStyle>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QComboBox>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextList>
#include <QTextListFormat>
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextBlockFormat>
#include <QLineEdit>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QRegExp>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QToolButton>
#include <QProgressBar>
#include <QPainter>
#include <QRegularExpression>
#include <QShortcut>
#include <algorithm>
#include <QMimeData>
#include <QDataStream>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDebug>
#include "../db/DatabaseManager.h"
#include "NoteListDelegate.h"
#include "../utils/Roles.h"
#include "TextEditor.h"
#include "SettingsDialog.h"
#include "../sync/SyncManager.h"
#include "../sync/ConfigLoader.h"
#include "GoogleAuthDialog.h"

namespace {
static QString readResourceText(const QString &path) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(f.readAll());
    }
    return {};
}

// Create modern icons using Unicode symbols
QIcon createIcon(const QString &symbol, const QColor &color = QColor(224, 224, 224)) {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.setPen(color);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, symbol);
    
    return QIcon(pixmap);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_mainSplitter(nullptr),
      m_folderTree(nullptr),
      m_noteList(nullptr),
      m_textEditor(nullptr),
  
      m_currentNoteId(-1),
      m_currentFolderId(-1),
      m_autoSaveTimer(new QTimer(this)),
      m_autoSaveEnabled(true),
      m_folderModel(new QStandardItemModel(this)),
      m_notesModel(new NotesModel(this)),
      m_syncManager(nullptr) {
    setWindowTitle("Notes - Orchard");
    setMinimumSize(1200, 800);  // Increased minimum size for all toolbar buttons
    resize(1400, 900);  // Set default window size
    
    // Set WM_CLASS for proper desktop integration
    setObjectName("Notes");
    setProperty("WM_CLASS", "Notes");
    
    // Set the window title to match the expected WM_CLASS
    setWindowTitle("Notes");
    
    // Set a unique window identifier for proper desktop integration
    setWindowFilePath("notes://");
    setWindowRole("notes-editor");
    
    // Set a unique window identifier for better desktop integration
    // Note: _NET_WM_PID is automatically set by Qt
    
    // Set X11 window hints for proper desktop integration (Qt5 compatible)
    #ifdef Q_OS_LINUX
    // Window manager integration is handled via window title, icon, and WM_CLASS
    // Set the WM_CLASS property explicitly
    setProperty("_NET_WM_CLASS", "Notes");
    #endif
    
    // Set window icon using multiple fallback methods
    QIcon windowIcon;
    windowIcon = QIcon(":/icons/notes.svg");
    setWindowIcon(windowIcon);
    
    setupUi();
    setupStyle();
    setupDatabaseConnections();
    setupGoogleDriveSync();
    
    // Set window title bar colors for Linux
    #ifdef Q_OS_LINUX
    setWindowFlags(windowFlags() | Qt::Window);
    #endif
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    // Enhanced toolbar with modern icons
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolbar->setMinimumHeight(40);  // Ensure toolbar has enough height
    addToolBar(Qt::TopToolBarArea, m_toolbar);
    
    // Folder actions (leftmost)
    m_actNewFolder = m_toolbar->addAction(createIcon("📁"), "New Folder", this, &MainWindow::createNewFolder);
    m_actNewFolder->setToolTip("Create a new folder");
    
    m_actDeleteFolder = m_toolbar->addAction(createIcon("🗑"), "Delete Folder", this, &MainWindow::deleteSelectedFolder);
    m_actDeleteFolder->setToolTip("Delete selected folder");
    
    m_toolbar->addSeparator();
    
    // Note actions with modern icons
    m_actNewNote = m_toolbar->addAction(createIcon("+"), "New Note", this, &MainWindow::createNewNote);
    m_actNewNote->setShortcut(QKeySequence::New);
    m_actNewNote->setToolTip("Create a new note (Ctrl+N)");
    
    m_actDeleteNote = m_toolbar->addAction(createIcon("🗑"), "Delete", this, &MainWindow::deleteSelectedNote);
    m_actDeleteNote->setShortcut(QKeySequence::Delete);
    m_actDeleteNote->setToolTip("Delete selected note (Del)");
    

    
    m_toolbar->addSeparator();
    
    // Settings action
    m_actSettings = m_toolbar->addAction(createIcon("⚙"), "Settings", this, &MainWindow::showSettings);
    m_actSettings->setToolTip("Open settings");
    
    m_toolbar->addSeparator();
    
    // Google Drive Sync actions
    m_actConnectGoogleDrive = m_toolbar->addAction(createIcon("☁"), "Connect to Google Drive", this, &MainWindow::onGoogleDriveConnect);
    m_actConnectGoogleDrive->setToolTip("Connect to Google Drive for cloud sync");
    
    m_actSyncNow = m_toolbar->addAction(createIcon("🔄"), "Sync Now", this, &MainWindow::onSyncNow);
    m_actSyncNow->setToolTip("Sync notes with Google Drive now");
    m_actSyncNow->setEnabled(false);
    m_actSyncNow->setVisible(false);  // Initially hidden
    
    m_actSyncSettings = m_toolbar->addAction(createIcon("⚙"), "Sync Settings", this, &MainWindow::onSyncSettings);
    m_actSyncSettings->setToolTip("Configure Google Drive sync settings");
    m_actSyncSettings->setEnabled(false);
    m_actSyncSettings->setVisible(false);  // Initially hidden
    
    // Debug: Check if actions were created
    qDebug() << "Sync actions created:";
    qDebug() << "  Connect action:" << (m_actConnectGoogleDrive ? "YES" : "NO");
    qDebug() << "  Sync Now action:" << (m_actSyncNow ? "YES" : "NO");
    qDebug() << "  Sync Settings action:" << (m_actSyncSettings ? "YES" : "NO");
    qDebug() << "  Toolbar actions count:" << m_toolbar->actions().count();
    
    // Debug: Check action properties
    if (m_actSyncNow) {
        qDebug() << "  Sync Now action text:" << m_actSyncNow->text();
        qDebug() << "  Sync Now action visible:" << m_actSyncNow->isVisible();
        qDebug() << "  Sync Now action enabled:" << m_actSyncNow->isEnabled();
    }
    
    m_toolbar->addSeparator();
    
    // View toggle button (for future preview mode)
    auto *viewToggle = new QToolButton(m_toolbar);
    viewToggle->setText("👁");
    viewToggle->setToolTip("Toggle preview mode");
    viewToggle->setCheckable(true);
    m_toolbar->addWidget(viewToggle);
    
    // Debug: List all toolbar actions
    qDebug() << "All toolbar actions:";
    for (QAction *action : m_toolbar->actions()) {
        qDebug() << "  -" << action->text() << "(" << action->isVisible() << "enabled:" << action->isEnabled() << ")";
    }
    
    // Debug: Check if sync actions are in toolbar
    qDebug() << "Sync actions in toolbar:";
    qDebug() << "  Connect action in toolbar:" << m_toolbar->actions().contains(m_actConnectGoogleDrive);
    qDebug() << "  Sync Now action in toolbar:" << m_toolbar->actions().contains(m_actSyncNow);
    qDebug() << "  Sync Settings action in toolbar:" << m_toolbar->actions().contains(m_actSyncSettings);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    // Left panel: Folders with header
    auto *leftPanel = new QWidget(m_mainSplitter);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    // Folder header
    auto *folderHeader = new QLabel("Folders", leftPanel);
    folderHeader->setStyleSheet("background: #2d2d2d; color: #e0e0e0; padding: 8px 12px; font-weight: 600; border-bottom: 1px solid #404040;");
    leftLayout->addWidget(folderHeader);
    
    m_folderTree = new QTreeView(leftPanel);
    m_folderTree->setHeaderHidden(true);
    m_folderTree->setUniformRowHeights(true);
    m_folderTree->setSortingEnabled(false);
    m_folderTree->setAnimated(true);
    m_folderTree->setExpandsOnDoubleClick(true);
    m_folderTree->setIndentation(16);
    
    // Enable drop for folders
    m_folderTree->setAcceptDrops(true);
    m_folderTree->setDropIndicatorShown(true);
    leftLayout->addWidget(m_folderTree, 1);
    leftPanel->setLayout(leftLayout);

    // Middle panel: Notes list with header
    auto *middlePanel = new QWidget(m_mainSplitter);
    auto *middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);
    
    // Notes header
    auto *notesHeader = new QLabel("Notes", middlePanel);
    notesHeader->setStyleSheet("background: #2d2d2d; color: #e0e0e0; padding: 8px 12px; font-weight: 600; border-bottom: 1px solid #404040;");
    middleLayout->addWidget(notesHeader);
    
    m_noteList = new QListView(middlePanel);
    m_noteList->setObjectName("NotesList");
    m_noteList->setUniformItemSizes(true);
    m_noteList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_noteList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_noteList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_noteList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_noteList->setSpacing(4);
    m_noteList->setItemDelegate(new NoteListDelegate(m_noteList));
    
    // Enable drag and drop for notes
    m_noteList->setDragEnabled(true);
    m_noteList->setDragDropMode(QAbstractItemView::DragOnly);
    m_noteList->setDefaultDropAction(Qt::MoveAction);
    middleLayout->addWidget(m_noteList, 1);
    middlePanel->setLayout(middleLayout);

    // Right panel: Editor with header
    auto *rightPanel = new QWidget(m_mainSplitter);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    // Editor header
    auto *editorHeader = new QLabel("Editor", rightPanel);
    editorHeader->setStyleSheet("background: #2d2d2d; color: #e0e0e0; padding: 8px 12px; font-weight: 600; border-bottom: 1px solid #404040;");
    rightLayout->addWidget(editorHeader);
    
    // Text Editor
    m_textEditor = new TextEditor(rightPanel);
    m_textEditor->setPlaceholderText("Start typing your note...");
    m_textEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_textEditor->setTabStopDistance(28);
    m_textEditor->setFont(QFont(QApplication::font().family(), 13));

    rightLayout->addWidget(m_textEditor, 1);
    rightPanel->setLayout(rightLayout);

    // Add panels to splitter with better proportions
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(middlePanel);
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setChildrenCollapsible(false);

    // Better default sizes
    QList<int> sizes;
    sizes << 220 << 300 << 600;
    m_mainSplitter->setSizes(sizes);

    // Enhanced status bar
    auto *statusBar = this->statusBar();
    statusBar->setStyleSheet("QStatusBar::item { border: none; }");
    
    // Word count label
    auto *wordCountLabel = new QLabel("Words: 0", statusBar);
    wordCountLabel->setStyleSheet("color: #999999; padding: 0 8px;");
    statusBar->addPermanentWidget(wordCountLabel);
    
    // Character count label
    auto *charCountLabel = new QLabel("Chars: 0", statusBar);
    charCountLabel->setStyleSheet("color: #999999; padding: 0 8px;");
    statusBar->addPermanentWidget(charCountLabel);
    
    // Line count label
    auto *lineCountLabel = new QLabel("Lines: 1", statusBar);
    lineCountLabel->setStyleSheet("color: #999999; padding: 0 8px;");
    statusBar->addPermanentWidget(lineCountLabel);
    
    // Sync status label
    auto *syncStatusLabel = new QLabel("Sync: Not Connected", statusBar);
    syncStatusLabel->setStyleSheet("color: #999999; padding: 0 8px;");
    statusBar->addPermanentWidget(syncStatusLabel);
    syncStatusLabel->setObjectName("syncStatusLabel");

    // Initialize database
    if (DatabaseManager::instance().open()) {
        DatabaseManager::instance().initializeSchema();
        statusBar->showMessage("Database connected", 3000);
    } else {
        statusBar->showMessage("Database connection failed", 5000);
    }

    // Text change detection
    connect(m_textEditor, &QTextEdit::textChanged, this, [this]() {
        if (m_currentNoteIndex.isValid()) {
            m_noteModified = true;
        }
    });

    // Update word/character count
    connect(m_textEditor, &QTextEdit::textChanged, this, [this, wordCountLabel, charCountLabel, lineCountLabel]() {
        QString text = m_textEditor->toPlainText();
        int wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
        int charCount = text.length();
        int lineCount = text.count('\n') + 1;
        
        wordCountLabel->setText(QString("Words: %1").arg(wordCount));
        charCountLabel->setText(QString("Chars: %1").arg(charCount));
        lineCountLabel->setText(QString("Lines: %1").arg(lineCount));
    });

    // Load folders from database
    loadFoldersFromDatabase();
    
    // Connect folder selection to update notes
    connect(m_folderTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onFolderSelected);
    
    // Enable drop event handling for folder tree
    m_folderTree->viewport()->installEventFilter(this);
    
    // Select the first folder by default
    if (m_folderModel->rowCount() > 0) {
        QModelIndex firstFolder = m_folderModel->index(0, 0);
        m_folderTree->setCurrentIndex(firstFolder);
        onFolderSelected(firstFolder);
    }

    // Apply folder icons to the model
    const QIcon folderIcon = style()->standardIcon(QStyle::SP_DirIcon);
    for (int i = 0; i < m_folderModel->rowCount(); ++i) {
        QStandardItem *item = m_folderModel->item(i);
        if (item) {
            item->setIcon(folderIcon);
        }
    }

    // Note model will be created when a folder is selected
    
    // Set a proper font for the note list
    QFont listFont = QApplication::font();
    if (listFont.pointSize() <= 0) {
        listFont = QFont("Arial", 11);
    }
    m_noteList->setFont(listFont);
    


    // Enhanced note selection with smooth transitions
    connect(m_noteList->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &previous) {
                // Save previous note if modified
                if (m_noteModified && m_currentNoteIndex.isValid()) {
                    saveCurrentNote();
                }
                
                if (!current.isValid()) {
                    m_currentNoteIndex = QModelIndex();
                    m_noteModified = false;
            
                    return;
                }
                
                m_currentNoteIndex = current;
                m_noteModified = false;
                loadNoteContent(current);
                
                

                // Smooth fade animation
                    auto *effect = new QGraphicsOpacityEffect(m_textEditor);
    m_textEditor->setGraphicsEffect(effect);
    auto *fade = new QPropertyAnimation(effect, "opacity", m_textEditor);
                fade->setDuration(200);
                fade->setStartValue(0.3);
                fade->setEndValue(1.0);
                fade->setEasingCurve(QEasingCurve::OutCubic);
                connect(fade, &QPropertyAnimation::finished, effect, &QObject::deleteLater);
                fade->start(QAbstractAnimation::DeleteWhenStopped);
            });


    
    setupContextMenus();
}

void MainWindow::setupContextMenus() {
    // Folder tree context menu
    m_folderTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_folderTree, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background: #2d2d2d; border: 1px solid #404040; border-radius: 8px; padding: 4px 0px; } "
                          "QMenu::item { padding: 8px 16px; border-radius: 4px; margin: 1px 4px; color: #e0e0e0; } "
                          "QMenu::item:selected { background: rgba(0, 122, 255, 0.2); color: #ffffff; } "
                          "QMenu::separator { height: 1px; background: #404040; margin: 4px 8px; }");
        
        QAction *newFolderAction = menu.addAction("📁 New Folder");
        newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
        
        menu.addSeparator();
        
        QAction *renameAction = menu.addAction("✏️ Rename");
        renameAction->setShortcut(QKeySequence("F2"));
        
        QAction *deleteAction = menu.addAction("🗑️ Delete Folder");
        deleteAction->setShortcut(QKeySequence::Delete);
        
        menu.addSeparator();
        
        QAction *expandAllAction = menu.addAction("📂 Expand All");
        QAction *collapseAllAction = menu.addAction("📁 Collapse All");
        
        menu.addSeparator();
        
        QAction *importAction = menu.addAction("📥 Import Markdown Files");
        
        // Enable/disable actions based on selection
        QModelIndex index = m_folderTree->indexAt(pos);
        bool hasSelection = index.isValid();
        renameAction->setEnabled(hasSelection);
        deleteAction->setEnabled(hasSelection);
        
        QAction *selectedAction = menu.exec(m_folderTree->mapToGlobal(pos));
        
        if (selectedAction == newFolderAction) {
            createNewFolder();
        } else if (selectedAction == renameAction) {
            // TODO: Implement rename functionality
            QMessageBox::information(this, "Rename", "Rename functionality coming soon!");
        } else if (selectedAction == deleteAction) {
            deleteSelectedFolder();
        } else if (selectedAction == expandAllAction) {
            m_folderTree->expandAll();
        } else if (selectedAction == collapseAllAction) {
            m_folderTree->collapseAll();
        } else if (selectedAction == importAction) {
            manualImportMarkdownFiles();
        }
    });
    
    // Notes list context menu
    m_noteList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_noteList, &QListView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background: #2d2d2d; border: 1px solid #404040; border-radius: 8px; padding: 4px 0px; } "
                          "QMenu::item { padding: 8px 16px; border-radius: 4px; margin: 1px 4px; color: #e0e0e0; } "
                          "QMenu::item:selected { background: rgba(0, 122, 255, 0.2); color: #ffffff; } "
                          "QMenu::separator { height: 1px; background: #404040; margin: 4px 8px; }");
        
        QAction *newNoteAction = menu.addAction("📝 New Note");
        newNoteAction->setShortcut(QKeySequence::New);
        
        menu.addSeparator();
        
        QAction *duplicateAction = menu.addAction("📋 Duplicate");
        duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
        
        menu.addSeparator();
        
        QAction *deleteAction = menu.addAction("🗑️ Delete Note");
        deleteAction->setShortcut(QKeySequence::Delete);
        
        // Enable/disable actions based on selection
        QModelIndex index = m_noteList->indexAt(pos);
        bool hasSelection = index.isValid();
        duplicateAction->setEnabled(hasSelection);
        deleteAction->setEnabled(hasSelection);
        
        if (hasSelection) {
            // Note-specific actions when a note is selected
        }
        
        QAction *selectedAction = menu.exec(m_noteList->mapToGlobal(pos));
        
        if (selectedAction == newNoteAction) {
            createNewNote();
        } else if (selectedAction == duplicateAction) {
            // TODO: Implement duplicate functionality
            QMessageBox::information(this, "Duplicate", "Duplicate functionality coming soon!");
        } else if (selectedAction == deleteAction) {
            deleteSelectedNote();
        }
    });
    
    // Editor context menu
    m_textEditor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEditor, &QTextEdit::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        menu.setStyleSheet("QMenu { background: #2d2d2d; border: 1px solid #404040; border-radius: 8px; padding: 4px 0px; } "
                          "QMenu::item { padding: 8px 16px; border-radius: 4px; margin: 1px 4px; color: #e0e0e0; } "
                          "QMenu::item:selected { background: rgba(0, 122, 255, 0.2); color: #ffffff; } "
                          "QMenu::separator { height: 1px; background: #404040; margin: 4px 8px; }");
        
        // Standard text editing actions
        QAction *undoAction = menu.addAction("↶ Undo");
        undoAction->setShortcut(QKeySequence::Undo);
        undoAction->setEnabled(m_textEditor->document()->isUndoAvailable());
        
        QAction *redoAction = menu.addAction("↷ Redo");
        redoAction->setShortcut(QKeySequence::Redo);
        redoAction->setEnabled(m_textEditor->document()->isRedoAvailable());
        
        menu.addSeparator();
        
        QAction *cutAction = menu.addAction("✂️ Cut");
        cutAction->setShortcut(QKeySequence::Cut);
        cutAction->setEnabled(m_textEditor->textCursor().hasSelection());
        
        QAction *copyAction = menu.addAction("📋 Copy");
        copyAction->setShortcut(QKeySequence::Copy);
        copyAction->setEnabled(m_textEditor->textCursor().hasSelection());
        
        QAction *pasteAction = menu.addAction("📋 Paste");
        pasteAction->setShortcut(QKeySequence::Paste);
        
        menu.addSeparator();
        
        // Text editing actions
        QAction *selectAllAction = menu.addAction("Select All");
        selectAllAction->setShortcut(QKeySequence::SelectAll);
        
        QAction *duplicateAction = menu.addAction("Duplicate");
        duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
        
        menu.addSeparator();
        
        QAction *upperAction = menu.addAction("UPPERCASE");
        QAction *lowerAction = menu.addAction("lowercase");
        QAction *titleAction = menu.addAction("Title Case");
        
        menu.addSeparator();
        
        QAction *taskAction = menu.addAction("- [ ] Task");
        
        QAction *selectedAction = menu.exec(m_textEditor->mapToGlobal(pos));
        
        // Handle text editing actions
        if (selectedAction == undoAction) {
            m_textEditor->undo();
        } else if (selectedAction == redoAction) {
            m_textEditor->redo();
        } else if (selectedAction == cutAction) {
            m_textEditor->cut();
        } else if (selectedAction == copyAction) {
            m_textEditor->copy();
        } else if (selectedAction == pasteAction) {
            m_textEditor->paste();
        }
        // Handle basic text actions
        else if (selectedAction == selectAllAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            cursor.select(QTextCursor::Document);
            m_textEditor->setTextCursor(cursor);
        } else if (selectedAction == duplicateAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            QString selectedText = cursor.selectedText();
            if (selectedText.isEmpty()) {
                cursor.select(QTextCursor::LineUnderCursor);
                selectedText = cursor.selectedText();
            }
            cursor.insertText(selectedText + selectedText);
        } else if (selectedAction == upperAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            QString selectedText = cursor.selectedText();
            cursor.insertText(selectedText.toUpper());
        } else if (selectedAction == lowerAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            QString selectedText = cursor.selectedText();
            cursor.insertText(selectedText.toLower());
        } else if (selectedAction == titleAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            QString selectedText = cursor.selectedText();
            cursor.insertText(selectedText.toLower().replace(0, 1, selectedText[0].toUpper()));
        } else if (selectedAction == taskAction) {
            QTextCursor cursor = m_textEditor->textCursor();
            cursor.insertText("- [ ] ");
        }
    });
}

void MainWindow::setupStyle() {
    // Try to set SF Pro if available, otherwise fallback gracefully
    const QStringList preferredFamilies = {
        QStringLiteral("SF Pro Text"),
        QStringLiteral("SF Pro Display"),
        QStringLiteral("San Francisco"),
        QStringLiteral("Helvetica Neue"),
        QStringLiteral("Noto Sans"),
        QStringLiteral("Inter"),
        QApplication::font().family()
    };

    QFont baseFont;
    for (const auto &fam : preferredFamilies) {
        QFont f(fam);
        if (QFontInfo(f).family() == fam) { baseFont = f; break; }
    }
    if (baseFont.family().isEmpty()) {
        baseFont = QApplication::font();
    }
    baseFont.setPointSize(12);
    QApplication::setFont(baseFont);

    // Dark palette (Apple Notes dark-like)
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(28, 28, 28));
    palette.setColor(QPalette::Base, QColor(34, 34, 34));
    palette.setColor(QPalette::AlternateBase, QColor(40, 40, 40));
    palette.setColor(QPalette::Text, QColor(230, 230, 230));
    palette.setColor(QPalette::ButtonText, QColor(230, 230, 230));
    palette.setColor(QPalette::Button, QColor(34, 34, 34));
    palette.setColor(QPalette::Highlight, QColor(255, 214, 10));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    palette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
    palette.setColor(QPalette::ToolTipText, QColor(230, 230, 230));
    
    // Set title bar colors
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    palette.setColor(QPalette::Light, QColor(255, 255, 255));
    palette.setColor(QPalette::Midlight, QColor(255, 255, 255));
    palette.setColor(QPalette::Dark, QColor(255, 255, 255));
    palette.setColor(QPalette::Mid, QColor(255, 255, 255));
    palette.setColor(QPalette::Shadow, QColor(255, 255, 255));
    
    QApplication::setPalette(palette);

    // Stylesheet
    const QString qss = readResourceText(":/styles/app.qss");
    if (!qss.isEmpty()) {
        if (qApp) qApp->setStyleSheet(qss);
    }
}

void MainWindow::createNewNote() {
    createNoteInCurrentFolder();
}

void MainWindow::createNoteInCurrentFolder() {
    if (m_currentFolderId <= 0) {
        // If no folder is selected, select the first folder
        if (m_folderModel->rowCount() > 0) {
            QModelIndex firstFolder = m_folderModel->index(0, 0);
            onFolderSelected(firstFolder);
            m_folderTree->setCurrentIndex(firstFolder);
        } else {
            return; // No folders available
        }
    }
    
    DatabaseManager &db = DatabaseManager::instance();
    int noteId = db.createNote(m_currentFolderId, "Untitled", "");
    
    if (noteId > 0) {
        // Reload notes to show the new note
        loadNotesFromDatabase(m_currentFolderId);
        
        // Select the new note (it should be at the top)
        if (m_notesModel->rowCount() > 0) {
            QModelIndex newNoteIndex = m_notesModel->index(0, 0);
            m_noteList->setCurrentIndex(newNoteIndex);
    m_textEditor->setFocus();
        }
    }
}

void MainWindow::deleteSelectedNote() {
    QModelIndex current = m_noteList->currentIndex();
    if (!current.isValid()) return;
    
    QString noteTitle = current.data(Qt::DisplayRole).toString();
    int noteId = current.data(Qt::UserRole).toInt();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Note", 
        QString("Are you sure you want to delete '%1'?").arg(noteTitle),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        DatabaseManager &db = DatabaseManager::instance();
        if (db.deleteNote(noteId)) {
            loadNotesFromDatabase(m_currentFolderId);
        }
    }
}

void MainWindow::createNewFolder() {
    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", 
        "Folder name:", QLineEdit::Normal, "Untitled Folder", &ok);
    
    if (ok && !folderName.isEmpty()) {
        DatabaseManager &db = DatabaseManager::instance();
        int folderId = db.createFolder(folderName);
        if (folderId > 0) {
            loadFoldersFromDatabase();
        }
    }
}

void MainWindow::deleteSelectedFolder() {
    QModelIndex current = m_folderTree->currentIndex();
    if (!current.isValid()) return;
    
    QString folderName = current.data(Qt::DisplayRole).toString();
    int folderId = current.data(Qt::UserRole).toInt();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Folder", 
        QString("Are you sure you want to delete folder '%1' and all its contents?").arg(folderName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        DatabaseManager &db = DatabaseManager::instance();
        if (db.deleteFolder(folderId)) {
            loadFoldersFromDatabase();
        }
    }
}



void MainWindow::saveCurrentNote() {
    if (m_currentNoteId <= 0) return;
    
    QString content = m_textEditor->toPlainText();
    
    // Extract title from first line
    QString title = "Untitled";
    QStringList lines = content.split('\n');
    if (!lines.isEmpty()) {
        QString firstLine = lines[0].trimmed();
        if (firstLine.startsWith("# ")) {
            // If it's a markdown heading, extract the title
            title = firstLine.mid(2).trimmed();
        } else if (!firstLine.isEmpty()) {
            // If it's not a heading but has content, use it as title
            title = firstLine;
        }
        if (title.isEmpty()) title = "Untitled";
    }
    
    DatabaseManager &db = DatabaseManager::instance();
    if (db.updateNote(m_currentNoteId, title, content)) {
        // Reload notes to update the list
        loadNotesFromDatabase(m_currentFolderId);
        m_noteModified = false;
    }
}

void MainWindow::loadNoteContent(const QModelIndex &index) {
    if (!index.isValid()) return;
    
    int noteId = index.data(Qt::UserRole).toInt();
    if (noteId <= 0) return;
    
    DatabaseManager &db = DatabaseManager::instance();
    NoteData note = db.getNote(noteId);
    
    if (note.id > 0) {
        m_currentNoteId = note.id;
        m_textEditor->setPlainText(note.body);
    }
}



void MainWindow::setupDatabaseConnections() {
    DatabaseManager &db = DatabaseManager::instance();
    
    // Connect database signals
    connect(&db, &DatabaseManager::noteSaved, this, &MainWindow::onNoteSaved);
    connect(&db, &DatabaseManager::noteDeleted, this, &MainWindow::onNoteDeleted);
    connect(&db, &DatabaseManager::folderSaved, this, &MainWindow::onFolderSaved);
    connect(&db, &DatabaseManager::folderDeleted, this, &MainWindow::onFolderDeleted);
    connect(&db, &DatabaseManager::autoSaveTriggered, this, &MainWindow::onAutoSaveTriggered);
    
    // Setup auto-save timer
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::onAutoSaveTimeout);
    m_autoSaveTimer->setSingleShot(true);
    
    // Connect text changes to auto-save
    connect(m_textEditor, &QTextEdit::textChanged, this, &MainWindow::onTextChanged);
}

void MainWindow::loadFoldersFromDatabase() {
    DatabaseManager &db = DatabaseManager::instance();
    db.populateFolderModel(m_folderModel);
    m_folderTree->setModel(m_folderModel);
    m_folderTree->expandAll();
    
    // Apply folder icons after loading
    const QIcon folderIcon = style()->standardIcon(QStyle::SP_DirIcon);
    for (int i = 0; i < m_folderModel->rowCount(); ++i) {
        QStandardItem *item = m_folderModel->item(i);
        if (item) {
            item->setIcon(folderIcon);
        }
    }
}

void MainWindow::loadNotesFromDatabase(int folderId) {
    DatabaseManager &db = DatabaseManager::instance();
    db.populateNotesModel(m_notesModel, folderId);
    m_noteList->setModel(m_notesModel);
}

void MainWindow::scheduleAutoSave() {
    if (m_autoSaveEnabled && m_noteModified && m_currentNoteId > 0) {
        m_autoSaveTimer->start(2000); // 2 seconds delay
    }
}

void MainWindow::importReadmeFiles() {
    DatabaseManager &db = DatabaseManager::instance();
    QString notesDir = db.getNotesDirectory();
    db.importReadmeFiles(notesDir);
    
    // Reload notes if we're in the Imported folder
    if (m_currentFolderId > 0) {
        loadNotesFromDatabase(m_currentFolderId);
    }
}

void MainWindow::manualImportMarkdownFiles() {
    DatabaseManager &db = DatabaseManager::instance();
    QString notesDir = db.getNotesDirectory();
    
    // Force import even if auto-import is disabled
    db.manualImportMarkdownFiles();
    
    // Reload folders to show any new "Imported" folder
    loadFoldersFromDatabase();
    
    // Reload notes if we're in the Imported folder
    if (m_currentFolderId > 0) {
        loadNotesFromDatabase(m_currentFolderId);
    }
    
    statusBar()->showMessage("Markdown files imported successfully", 3000);
}

void MainWindow::onNoteSaved(int noteId) {
    if (noteId == m_currentNoteId) {
        m_noteModified = false;
        statusBar()->showMessage("Note saved", 2000);
    }
}

void MainWindow::onNoteDeleted(int noteId) {
    if (noteId == m_currentNoteId) {
        m_currentNoteId = -1;
        m_currentNoteIndex = QModelIndex();
        m_noteModified = false;
        m_textEditor->clear();
    }
}

void MainWindow::onFolderSaved(int folderId) {
    loadFoldersFromDatabase();
}

void MainWindow::onFolderDeleted(int folderId) {
    if (folderId == m_currentFolderId) {
        m_currentFolderId = -1;
        m_currentFolderIndex = QModelIndex();
        m_currentNoteId = -1;
        m_currentNoteIndex = QModelIndex();
        m_noteModified = false;
        m_textEditor->clear();
    }
    loadFoldersFromDatabase();
}

void MainWindow::onAutoSaveTriggered() {
    if (m_noteModified && m_currentNoteId > 0) {
        saveCurrentNote();
    }
}

void MainWindow::onTextChanged() {
    if (m_currentNoteIndex.isValid()) {
        m_noteModified = true;
        scheduleAutoSave();
    }
}

void MainWindow::onAutoSaveTimeout() {
    if (m_noteModified && m_currentNoteId > 0) {
        saveCurrentNote();
    }
}

void MainWindow::showSettings() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        DatabaseManager &db = DatabaseManager::instance();
        db.setNotesDirectory(dialog.getNotesDirectory());
        db.enableAutoSave(dialog.isAutoSaveEnabled());
        db.setAutoSaveInterval(dialog.getAutoSaveInterval());
        db.setAutoImportEnabled(dialog.isAutoImportEnabled());
        
        m_autoSaveEnabled = dialog.isAutoSaveEnabled();
        
        // Import README files from the new directory (only if auto-import is enabled)
        if (dialog.isAutoImportEnabled()) {
            importReadmeFiles();
        }
        
        statusBar()->showMessage("Settings saved", 3000);
    }
}



void MainWindow::onFolderSelected(const QModelIndex &index) {
    if (!index.isValid()) return;
    
    // Save current note if modified
    if (m_noteModified && m_currentNoteId > 0) {
        saveCurrentNote();
    }
    
    m_currentFolderIndex = index;
    m_currentFolderId = index.data(Qt::UserRole).toInt();
    m_currentNoteIndex = QModelIndex();
    m_currentNoteId = -1;
    m_noteModified = false;
    
    // Load notes from database for this folder
    loadNotesFromDatabase(m_currentFolderId);
    
    // Clear the editor
    m_textEditor->clear();
    
    // Update pin button state
    
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_folderTree->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasFormat("application/x-notes-item")) {
                // Save the original folder selection
                m_originalFolderSelection = m_folderTree->currentIndex();
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::DragMove) {
            QDragMoveEvent *dragEvent = static_cast<QDragMoveEvent*>(event);
            if (dragEvent->mimeData()->hasFormat("application/x-notes-item")) {
                QPoint dragPos = dragEvent->pos();
                QModelIndex dragIndex = m_folderTree->indexAt(dragPos);
                
                if (dragIndex.isValid()) {
                    QByteArray itemData = dragEvent->mimeData()->data("application/x-notes-item");
                    QDataStream stream(itemData);
                    int noteId;
                    stream >> noteId;
                    
                    int targetFolderId = dragIndex.data(Qt::UserRole).toInt();
                    if (canDropNoteOnFolder(noteId, targetFolderId)) {
                        dragEvent->acceptProposedAction();
                        
                        // Highlight the folder being hovered over
                        m_folderTree->setCurrentIndex(dragIndex);
                        return true;
                    }
                }
                dragEvent->ignore();
                return true;
            }
        } else if (event->type() == QEvent::DragLeave) {
            // Restore the original folder selection when drag leaves
            restoreFolderSelection();
            return true;
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
            const QMimeData *mimeData = dropEvent->mimeData();
            
            if (mimeData->hasFormat("application/x-notes-item")) {
                QByteArray itemData = mimeData->data("application/x-notes-item");
                QDataStream stream(itemData);
                int noteId;
                stream >> noteId;
                
                // Get the drop position
                QPoint dropPos = dropEvent->pos();
                QModelIndex dropIndex = m_folderTree->indexAt(dropPos);
                
                if (dropIndex.isValid()) {
                    int targetFolderId = dropIndex.data(Qt::UserRole).toInt();
                    if (canDropNoteOnFolder(noteId, targetFolderId)) {
                        moveNoteToFolder(noteId, targetFolderId);
                        dropEvent->acceptProposedAction();
                        
                        // Restore the original folder selection
                        restoreFolderSelection();
                        return true;
                    }
                }
                
                // Restore the original folder selection if drop was invalid
                restoreFolderSelection();
            }
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::moveNoteToFolder(int noteId, int targetFolderId) {
    DatabaseManager &db = DatabaseManager::instance();
    
    // Get the note data
    NoteData note = db.getNote(noteId);
    if (note.id == -1) return;
    
    // Create the note in the new folder
    int newNoteId = db.createNote(targetFolderId, note.title, note.body);
    if (newNoteId > 0) {
        // Delete the note from the old folder
        db.deleteNote(noteId);
        
        // Reload the current folder's notes
        if (m_currentFolderId == note.folderId) {
            loadNotesFromDatabase(m_currentFolderId);
        }
        
        statusBar()->showMessage(QString("Note moved to %1").arg(db.getFolder(targetFolderId).name), 3000);
    }
}

bool MainWindow::canDropNoteOnFolder(int noteId, int targetFolderId) {
    DatabaseManager &db = DatabaseManager::instance();
    
    // Get the note data
    NoteData note = db.getNote(noteId);
    if (note.id == -1) return false;
    
    // Don't allow dropping on the same folder
    if (note.folderId == targetFolderId) return false;
    
    // Get the target folder data
    FolderData folder = db.getFolder(targetFolderId);
    if (folder.id == -1) return false;
    
    return true;
}

void MainWindow::restoreFolderSelection() {
    if (m_originalFolderSelection.isValid()) {
        m_folderTree->setCurrentIndex(m_originalFolderSelection);
        m_originalFolderSelection = QModelIndex();
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    
    // Ensure the window icon is set properly after the window is shown
    setWindowIcon(QIcon(":/icons/notes.svg"));
}

// Google Drive Sync Implementation
void MainWindow::setupGoogleDriveSync()
{
    // Ensure ConfigLoader is loaded first
    if (!ConfigLoader::instance().loadConfig()) {
        qWarning() << "Failed to load Google Drive configuration";
        qWarning() << "Errors:" << ConfigLoader::instance().getValidationErrors();
    }
    
    // Initialize the sync manager using the singleton DatabaseManager
    m_syncManager = new SyncManager(&DatabaseManager::instance(), this);
    
    // Connect sync manager signals
    connect(m_syncManager, &SyncManager::authenticationChanged, this, &MainWindow::onSyncStatusChanged);
    connect(m_syncManager, &SyncManager::syncStarted, this, &MainWindow::onSyncStatusChanged);
    connect(m_syncManager, &SyncManager::syncCompleted, this, &MainWindow::onSyncStatusChanged);
    connect(m_syncManager, &SyncManager::syncFailed, this, &MainWindow::onSyncError);
    
    // Connect to show success when folder is created
    
    
    // Update initial sync status
    onSyncStatusChanged();
}

void MainWindow::onGoogleDriveConnect()
{
    if (m_syncManager && m_syncManager->isAuthenticated()) {
        // Already connected - show disconnect option
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Google Drive", 
            "You are already connected to Google Drive. Would you like to disconnect?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            m_syncManager->logout();
        }
    } else {
        // Show the actual Google authentication dialog
        GoogleAuthDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString authCode = dialog.getAuthCode();
            
                    // Exchange auth code for tokens via SyncManager
        if (m_syncManager) {
            // Complete the OAuth flow by exchanging the auth code for tokens
            m_syncManager->completeOAuth(authCode);
        }
        }
    }
}

void MainWindow::onSyncNow()
{
    if (m_syncManager && !m_syncManager->isSyncing()) {
        qDebug() << "Sync Now button clicked - starting full sync";
        m_syncManager->syncAllNotes();
    }
}

void MainWindow::onSyncSettings()
{
    QMessageBox::information(this, "Sync Settings", "Google Drive sync settings:\n\n- Auto-sync: Every 15 minutes\n- Conflict resolution: Automatic\n- File format: Markdown (.md)\n\nFull settings dialog not yet implemented.");
}

void MainWindow::onSyncStatusChanged()
{
    if (!m_syncManager) return;
    
    bool isConnected = m_syncManager->isAuthenticated();
    bool isSyncing = m_syncManager->isSyncing();
    
    qDebug() << "MainWindow::onSyncStatusChanged - isConnected:" << isConnected << "isSyncing:" << isSyncing;
    
    // Update toolbar actions
    m_actConnectGoogleDrive->setText(isConnected ? "Disconnect from Google Drive" : "Connect to Google Drive");
    
    // Show/hide sync actions based on connection state
    m_actSyncNow->setVisible(isConnected);
    m_actSyncSettings->setVisible(isConnected);
    
    // Enable/disable sync actions based on connection and sync state
    bool shouldEnableSync = isConnected && !isSyncing;
    m_actSyncNow->setEnabled(shouldEnableSync);
    m_actSyncSettings->setEnabled(isConnected);
    
    qDebug() << "Sync Now button enabled:" << shouldEnableSync;
    qDebug() << "Sync Now action enabled state:" << m_actSyncNow->isEnabled();
    qDebug() << "Sync Now action visible state:" << m_actSyncNow->isVisible();
    qDebug() << "Sync actions visible:" << isConnected;
    
    // Force toolbar update
    m_toolbar->update();
    
    // Update status bar
    QString status = m_syncManager->getSyncStatus();
    statusBar()->showMessage(status);
    
    // Update sync status label
    QLabel *syncStatusLabel = findChild<QLabel*>("syncStatusLabel");
    if (syncStatusLabel) {
        syncStatusLabel->setText("Sync: " + status);
    }
}

void MainWindow::onSyncError(const QString &error)
{
    QMessageBox::warning(this, "Sync Error", "Google Drive sync failed:\n\n" + error);
    onSyncStatusChanged();
}


