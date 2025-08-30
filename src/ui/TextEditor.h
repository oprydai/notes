#pragma once

#include <QTextEdit>
#include <QCompleter>
#include <QTimer>
#include <QStringList>

class MarkdownEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit MarkdownEditor(QWidget *parent = nullptr);
    
    void setAutoSaveEnabled(bool enabled);
    void setAutoSaveInterval(int milliseconds);
    
signals:
    void contentChanged();
    void autoSaveRequested();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private slots:
    void onTextChanged();
    void onAutoSaveTimeout();
    void insertCompletion(const QString &completion);

private:
    void tryToggleCheckboxAtCursor(const QPoint &pos);
    void handleListAutoFormatting();
    void handleEnterInList();
    void handleTabIndentation(bool shiftPressed);
    QString getCurrentLineIndentation();
    void insertIndentedListItem(const QString &indent);
    bool handleMarkdownShortcuts(QKeyEvent *e);
    void insertMarkdownFormatting(const QString &format);
    void setupCompleter();
    void setupAutoSave();
    bool handleAutoCompletion(QKeyEvent *e);
    QString textUnderCursor() const;
    void insertLink();
    void insertImage();
    void insertTable();
    void insertCodeBlock();
    void insertQuote();
    void toggleBold();
    void toggleItalic();
    void toggleStrikethrough();
    void toggleCode();
    void insertHeader(int level);
    void insertHorizontalRule();
    void insertTaskList();
    void insertBulletList();
    void insertNumberedList();
    void formatSelection(const QString &before, const QString &after);
    QString getSelectedText();
    void replaceSelectedText(const QString &text);
    void insertAtCursor(const QString &text);
    void moveCursorToEndOfLine();
    void moveCursorToStartOfLine();
    void selectCurrentLine();
    void duplicateLine();
    void deleteLine();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    void insertTimestamp();
    void insertDate();
    void insertTime();
    
    // Auto-save functionality
    QTimer *m_autoSaveTimer;
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
    
    // Auto-completion
    QCompleter *m_completer;
    QStringList m_completionWords;
    
    // Editor state
    bool m_isModified;
    QString m_lastContent;
};


