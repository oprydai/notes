#include "MarkdownEditor.h"

#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QKeyEvent>
#include <QRegExp>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QTextDocument>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QFontMetrics>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QStringListModel>
#include <QAbstractItemView>

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QTextEdit(parent),
      m_autoSaveTimer(new QTimer(this)),
      m_autoSaveEnabled(true),
      m_autoSaveInterval(2000),
      m_completer(new QCompleter(this)),
      m_isModified(false) {
    
    setAcceptRichText(false);
    setupCompleter();
    setupAutoSave();
    
    // Connect text changes
    connect(this, &QTextEdit::textChanged, this, &MarkdownEditor::onTextChanged);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MarkdownEditor::onAutoSaveTimeout);
    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &MarkdownEditor::insertCompletion);
}

void MarkdownEditor::setAutoSaveEnabled(bool enabled) {
    m_autoSaveEnabled = enabled;
    if (!enabled) {
        m_autoSaveTimer->stop();
    }
}

void MarkdownEditor::setAutoSaveInterval(int milliseconds) {
    m_autoSaveInterval = milliseconds;
    if (m_autoSaveEnabled && m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

void MarkdownEditor::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        tryToggleCheckboxAtCursor(e->pos());
    }
    QTextEdit::mousePressEvent(e);
}

void MarkdownEditor::keyPressEvent(QKeyEvent *e) {
    // Handle auto-completion
    if (handleAutoCompletion(e)) {
        return;
    }
    
    // Handle markdown shortcuts first
    if (handleMarkdownShortcuts(e)) {
        return;
    }
    
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        handleEnterInList();
    } else if (e->key() == Qt::Key_Space) {
        handleListAutoFormatting();
    } else if (e->key() == Qt::Key_Tab) {
        handleTabIndentation(e->modifiers() & Qt::ShiftModifier);
        return; // Don't call parent to prevent default tab behavior
    } else if (e->key() == Qt::Key_D && e->modifiers() == Qt::ControlModifier) {
        duplicateLine();
        return;
    } else if (e->key() == Qt::Key_L && e->modifiers() == Qt::ControlModifier) {
        selectCurrentLine();
        return;
    } else if (e->key() == Qt::Key_Home && e->modifiers() == Qt::ControlModifier) {
        moveCursorToStartOfLine();
        return;
    } else if (e->key() == Qt::Key_End && e->modifiers() == Qt::ControlModifier) {
        moveCursorToEndOfLine();
        return;
    } else if (e->key() == Qt::Key_Up && e->modifiers() == Qt::AltModifier) {
        moveLineUp();
        return;
    } else if (e->key() == Qt::Key_Down && e->modifiers() == Qt::AltModifier) {
        moveLineDown();
        return;
    } else if (e->key() == Qt::Key_Slash && e->modifiers() == Qt::ControlModifier) {
        toggleComment();
        return;
    }
    
    QTextEdit::keyPressEvent(e);
}

void MarkdownEditor::focusInEvent(QFocusEvent *e) {
    QTextEdit::focusInEvent(e);
    // Restart auto-save timer if needed
    if (m_autoSaveEnabled && m_isModified) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

void MarkdownEditor::focusOutEvent(QFocusEvent *e) {
    QTextEdit::focusOutEvent(e);
    // Trigger auto-save immediately when losing focus
    if (m_autoSaveEnabled && m_isModified) {
        emit autoSaveRequested();
        m_autoSaveTimer->stop();
    }
}

void MarkdownEditor::onTextChanged() {
    m_isModified = true;
    emit contentChanged();
    
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start(m_autoSaveInterval);
    }
}

void MarkdownEditor::onAutoSaveTimeout() {
    if (m_isModified) {
        emit autoSaveRequested();
        m_isModified = false;
    }
}

void MarkdownEditor::insertCompletion(const QString &completion) {
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

void MarkdownEditor::setupCompleter() {
    // Common markdown words and phrases
    m_completionWords << "# " << "## " << "### " << "#### " << "##### " << "###### "
                     << "**" << "**" << "*" << "*" << "`" << "`"
                     << "```" << "```" << "> " << "- " << "1. "
                     << "---" << "===" << "[[" << "]]" << "![[" << "]]"
                     << "TODO:" << "FIXME:" << "NOTE:" << "WARNING:" << "IMPORTANT:"
                     << "https://" << "http://" << "mailto:" << "tel:"
                     << "![" << "](image.png)" << "[" << "](link)"
                     << "| " << "| --- |" << "| --- | --- |" << "| --- | --- | --- |"
                     << "`code`" << "```\n\n```" << "> quote" << "- [ ] " << "- [x] "
                     << "<!--" << "-->" << "~~" << "~~" << "++" << "++";
    
    m_completer->setModel(new QStringListModel(m_completionWords, m_completer));
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
}

void MarkdownEditor::setupAutoSave() {
    m_autoSaveTimer->setSingleShot(true);
}

bool MarkdownEditor::handleAutoCompletion(QKeyEvent *e) {
    if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (m_completer->popup()->isVisible()) {
            // Select the first completion
            if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                insertCompletion(m_completer->currentCompletion());
                m_completer->popup()->hide();
                return true;
            }
        }
    }
    
    // Show completion popup
    QString completionPrefix = textUnderCursor();
    if (completionPrefix.length() < 3) {
        m_completer->popup()->hide();
        return false;
    }
    
    m_completer->setCompletionPrefix(completionPrefix);
    m_completer->complete();
    return false;
}

QString MarkdownEditor::textUnderCursor() const {
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void MarkdownEditor::tryToggleCheckboxAtCursor(const QPoint &pos) {
    QTextCursor c = cursorForPosition(pos);
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    // Match markdown task list: - [ ] or - [x]
    if (line.startsWith("- [ ] ") || line.startsWith("- [x] ") || line.startsWith("- [X] ")) {
        if (line.startsWith("- [ ] ")) line.replace(3, 3, "[x]");
        else line.replace(3, 3, "[ ]");
        c.insertText(line);
    }
}

void MarkdownEditor::handleListAutoFormatting() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    // Auto-format bullet lists
    if (line.endsWith("- ")) {
        c.insertText("• ");
        return;
    }
    
    // Auto-format numbered lists
    QRegExp numPattern("^(\\d+)\\. $");
    if (numPattern.exactMatch(line)) {
        int currentNum = numPattern.cap(1).toInt();
        c.insertText(QString("%1. ").arg(currentNum + 1));
        return;
    }
}

void MarkdownEditor::handleEnterInList() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    // Handle task lists
    QRegExp taskPattern("^(- \\[([ xX])\\] )(.*)$");
    if (taskPattern.exactMatch(line)) {
        QString indent = getCurrentLineIndentation();
        QString checkbox = taskPattern.cap(2);
        QString content = taskPattern.cap(3);
        
        if (content.trimmed().isEmpty()) {
            // Empty task, remove it
            c.removeSelectedText();
            c.insertText("\n");
        } else {
            // Create new task
            c.insertText("\n" + indent + "- [ ] ");
        }
        return;
    }
    
    // Handle bullet lists
    QRegExp bulletPattern("^(- |\\* |\\+ |• )(.*)$");
    if (bulletPattern.exactMatch(line)) {
        QString indent = getCurrentLineIndentation();
        QString marker = bulletPattern.cap(1);
        QString content = bulletPattern.cap(2);
        
        if (content.trimmed().isEmpty()) {
            // Empty bullet, remove it
            c.removeSelectedText();
            c.insertText("\n");
        } else {
            // Create new bullet
            c.insertText("\n" + indent + marker);
        }
        return;
    }
    
    // Handle numbered lists
    QRegExp numPattern("^(\\d+)\\. (.*)$");
    if (numPattern.exactMatch(line)) {
        QString indent = getCurrentLineIndentation();
        int currentNum = numPattern.cap(1).toInt();
        QString content = numPattern.cap(2);
        
        if (content.trimmed().isEmpty()) {
            // Empty numbered item, remove it
            c.removeSelectedText();
            c.insertText("\n");
        } else {
            // Create new numbered item
            c.insertText(QString("\n%1%2. ").arg(indent).arg(currentNum + 1));
        }
        return;
    }
    
    // Default behavior
    QTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
}

void MarkdownEditor::handleTabIndentation(bool shiftPressed) {
    QTextCursor c = textCursor();
    QString indent = getCurrentLineIndentation();
    
    if (shiftPressed) {
        // Unindent
        if (indent.startsWith("    ")) {
            c.select(QTextCursor::LineUnderCursor);
            QString line = c.selectedText();
            c.insertText(line.mid(4));
        }
    } else {
        // Indent
        c.insertText("    ");
    }
}

QString MarkdownEditor::getCurrentLineIndentation() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    QString indent;
    for (int i = 0; i < line.length(); ++i) {
        if (line[i] == ' ' || line[i] == '\t') {
        indent += line[i];
        } else {
            break;
        }
    }
    return indent;
}

void MarkdownEditor::insertIndentedListItem(const QString &indent) {
    QTextCursor c = textCursor();
    c.insertText(indent + "- ");
}

bool MarkdownEditor::handleMarkdownShortcuts(QKeyEvent *e) {
    // Bold: Ctrl+B
    if (e->key() == Qt::Key_B && e->modifiers() == Qt::ControlModifier) {
        toggleBold();
        return true;
    }
    
    // Italic: Ctrl+I
    if (e->key() == Qt::Key_I && e->modifiers() == Qt::ControlModifier) {
        toggleItalic();
        return true;
    }
    
    // Strikethrough: Ctrl+S
    if (e->key() == Qt::Key_S && e->modifiers() == Qt::ControlModifier) {
        toggleStrikethrough();
        return true;
    }
    
    // Code: Ctrl+`
    if (e->key() == Qt::Key_QuoteLeft && e->modifiers() == Qt::ControlModifier) {
        toggleCode();
        return true;
    }
    
    // Headers: Ctrl+1-6
    if (e->modifiers() == Qt::ControlModifier) {
        switch (e->key()) {
            case Qt::Key_1: insertHeader(1); return true;
            case Qt::Key_2: insertHeader(2); return true;
            case Qt::Key_3: insertHeader(3); return true;
            case Qt::Key_4: insertHeader(4); return true;
            case Qt::Key_5: insertHeader(5); return true;
            case Qt::Key_6: insertHeader(6); return true;
        }
    }
    
    // Link: Ctrl+K
    if (e->key() == Qt::Key_K && e->modifiers() == Qt::ControlModifier) {
        insertLink();
        return true;
    }
    
    // Image: Ctrl+Shift+I
    if (e->key() == Qt::Key_I && e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        insertImage();
        return true;
    }
    
    // Table: Ctrl+T
    if (e->key() == Qt::Key_T && e->modifiers() == Qt::ControlModifier) {
        insertTable();
        return true;
    }
    
    // Code block: Ctrl+Shift+C
    if (e->key() == Qt::Key_C && e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        insertCodeBlock();
        return true;
    }
    
    // Quote: Ctrl+Q
    if (e->key() == Qt::Key_Q && e->modifiers() == Qt::ControlModifier) {
        insertQuote();
        return true;
    }
    
    // Horizontal rule: Ctrl+H
    if (e->key() == Qt::Key_H && e->modifiers() == Qt::ControlModifier) {
        insertHorizontalRule();
        return true;
    }
    
    // Task list: Ctrl+L
    if (e->key() == Qt::Key_L && e->modifiers() == Qt::ControlModifier) {
        insertTaskList();
        return true;
    }
    
    return false;
}

void MarkdownEditor::insertMarkdownFormatting(const QString &format) {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();
    
    if (selectedText.isEmpty()) {
        // Insert format markers and place cursor between them
        c.insertText(format);
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, format.length() / 2);
    } else {
        // Wrap selected text with format markers
        c.insertText(format + selectedText + format);
    }
    
    setTextCursor(c);
}

void MarkdownEditor::toggleBold() {
    formatSelection("**", "**");
}

void MarkdownEditor::toggleItalic() {
    formatSelection("*", "*");
}

void MarkdownEditor::toggleStrikethrough() {
    formatSelection("~~", "~~");
}

void MarkdownEditor::toggleCode() {
    formatSelection("`", "`");
}

void MarkdownEditor::insertHeader(int level) {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    // Remove existing header markers
    line = line.replace(QRegExp("^#{1,6}\\s*"), "");
    
    // Add new header marker
    QString headerMarker = QString(level, '#') + " ";
    c.insertText(headerMarker + line);
}

void MarkdownEditor::insertLink() {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();
    
    if (selectedText.isEmpty()) {
        c.insertText("[link text](url)");
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 4);
    } else {
        c.insertText("[" + selectedText + "](url)");
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 4);
    }
    
    setTextCursor(c);
}

void MarkdownEditor::insertImage() {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();
    
    if (selectedText.isEmpty()) {
        c.insertText("![alt text](image.png)");
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 9);
    } else {
        c.insertText("![" + selectedText + "](image.png)");
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 9);
    }
    
    setTextCursor(c);
}

void MarkdownEditor::insertTable() {
    QTextCursor c = textCursor();
    c.insertText("| Header 1 | Header 2 | Header 3 |\n");
    c.insertText("| --- | --- | --- |\n");
    c.insertText("| Cell 1 | Cell 2 | Cell 3 |\n");
    c.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, 2);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);
    setTextCursor(c);
}

void MarkdownEditor::insertCodeBlock() {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();
    
    if (selectedText.isEmpty()) {
        c.insertText("```\n\n```");
        c.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, 1);
    } else {
        c.insertText("```\n" + selectedText + "\n```");
    }
    
    setTextCursor(c);
}

void MarkdownEditor::insertQuote() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    if (line.startsWith("> ")) {
        // Remove quote
        c.insertText(line.mid(2));
    } else {
        // Add quote
        c.insertText("> " + line);
    }
}

void MarkdownEditor::insertHorizontalRule() {
    QTextCursor c = textCursor();
    c.insertText("\n---\n");
}

void MarkdownEditor::insertTaskList() {
    QTextCursor c = textCursor();
    c.insertText("- [ ] ");
}

void MarkdownEditor::insertBulletList() {
    QTextCursor c = textCursor();
    c.insertText("- ");
}

void MarkdownEditor::insertNumberedList() {
    QTextCursor c = textCursor();
    c.insertText("1. ");
}

void MarkdownEditor::formatSelection(const QString &before, const QString &after) {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();
    
    if (selectedText.isEmpty()) {
        c.insertText(before + after);
        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, after.length());
    } else {
        c.insertText(before + selectedText + after);
    }
    
    setTextCursor(c);
}

QString MarkdownEditor::getSelectedText() {
    return textCursor().selectedText();
}

void MarkdownEditor::replaceSelectedText(const QString &text) {
    QTextCursor c = textCursor();
    c.insertText(text);
}

void MarkdownEditor::insertAtCursor(const QString &text) {
    QTextCursor c = textCursor();
    c.insertText(text);
}

void MarkdownEditor::moveCursorToEndOfLine() {
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::EndOfLine);
    setTextCursor(c);
}

void MarkdownEditor::moveCursorToStartOfLine() {
    QTextCursor c = textCursor();
    c.movePosition(QTextCursor::StartOfLine);
    setTextCursor(c);
}

void MarkdownEditor::selectCurrentLine() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
        setTextCursor(c);
}

void MarkdownEditor::duplicateLine() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    c.movePosition(QTextCursor::EndOfLine);
    c.insertText("\n" + line);
}

void MarkdownEditor::deleteLine() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    c.removeSelectedText();
    c.deleteChar(); // Remove the newline
}

void MarkdownEditor::moveLineUp() {
    QTextCursor c = textCursor();
    int currentLine = c.blockNumber();
    if (currentLine > 0) {
        c.select(QTextCursor::LineUnderCursor);
        QString line = c.selectedText();
        c.removeSelectedText();
        c.deleteChar(); // Remove newline
        
        c.movePosition(QTextCursor::Up);
        c.movePosition(QTextCursor::StartOfLine);
        c.insertText(line + "\n");
        c.movePosition(QTextCursor::Up);
    }
}

void MarkdownEditor::moveLineDown() {
    QTextCursor c = textCursor();
    int currentLine = c.blockNumber();
    int totalLines = document()->blockCount();
    
    if (currentLine < totalLines - 1) {
        c.select(QTextCursor::LineUnderCursor);
        QString line = c.selectedText();
        c.removeSelectedText();
        c.deleteChar(); // Remove newline
        
        c.movePosition(QTextCursor::Down);
        c.movePosition(QTextCursor::StartOfLine);
        c.insertText(line + "\n");
        c.movePosition(QTextCursor::Up);
    }
}

void MarkdownEditor::toggleComment() {
    QTextCursor c = textCursor();
    c.select(QTextCursor::LineUnderCursor);
    QString line = c.selectedText();
    
    if (line.startsWith("<!--") && line.endsWith("-->")) {
        // Remove comment
        c.insertText(line.mid(4, line.length() - 7));
    } else {
        // Add comment
        c.insertText("<!--" + line + "-->");
    }
}

void MarkdownEditor::insertTimestamp() {
    QTextCursor c = textCursor();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    c.insertText(timestamp);
}

void MarkdownEditor::insertDate() {
    QTextCursor c = textCursor();
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    c.insertText(date);
}

void MarkdownEditor::insertTime() {
    QTextCursor c = textCursor();
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    c.insertText(time);
}


