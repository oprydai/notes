#include "TextEditor.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QTextBlock>

TextEditor::TextEditor(QWidget *parent)
    : QTextEdit(parent)
    , m_autoSaveTimer(new QTimer(this))
    , m_autoSaveEnabled(true)
    , m_autoSaveInterval(2)
{
    // Setup auto-save timer
    m_autoSaveTimer->setSingleShot(true);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &TextEditor::onAutoSaveTimeout);
    
    // Connect text changes
    connect(this, &QTextEdit::textChanged, this, &TextEditor::onTextChanged);
    
    // Set up basic styling
    setStyleSheet("QTextEdit { background: #1e1e1e; color: #e0e0e0; border: none; padding: 12px; font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace; font-size: 13px; line-height: 1.5; }");
}

void TextEditor::setAutoSaveEnabled(bool enabled)
{
    m_autoSaveEnabled = enabled;
}

void TextEditor::setAutoSaveInterval(int seconds)
{
    m_autoSaveInterval = seconds;
}

void TextEditor::keyPressEvent(QKeyEvent *event)
{
    // Handle basic shortcuts
    if (event->key() == Qt::Key_D && event->modifiers() == Qt::ControlModifier) {
        // Duplicate line
        QTextCursor cursor = textCursor();
        QString currentLine = cursor.block().text();
        cursor.movePosition(QTextCursor::EndOfLine);
        cursor.insertText("\n" + currentLine);
        return;
    }
    
    // Handle line selection
    if (event->key() == Qt::Key_L && event->modifiers() == Qt::ControlModifier) {
        QTextCursor cursor = textCursor();
        cursor.select(QTextCursor::LineUnderCursor);
        setTextCursor(cursor);
        return;
    }
    
    // Handle line movement
    if (event->key() == Qt::Key_Up && event->modifiers() == Qt::AltModifier) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up);
        cursor.movePosition(QTextCursor::StartOfLine);
        setTextCursor(cursor);
        return;
    }
    
    if (event->key() == Qt::Key_Down && event->modifiers() == Qt::AltModifier) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down);
        cursor.movePosition(QTextCursor::StartOfLine);
        setTextCursor(cursor);
        return;
    }
    
    // Default behavior
    QTextEdit::keyPressEvent(event);
}

void TextEditor::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
}

void TextEditor::focusOutEvent(QFocusEvent *event)
{
    // Auto-save when losing focus
    if (m_autoSaveEnabled) {
        emit autoSaveRequested();
    }
    QTextEdit::focusOutEvent(event);
}

void TextEditor::onTextChanged()
{
    emit contentChanged();
    
    if (m_autoSaveEnabled) {
        scheduleAutoSave();
    }
}

void TextEditor::onAutoSaveTimeout()
{
    if (m_autoSaveEnabled) {
        emit autoSaveRequested();
    }
}

void TextEditor::scheduleAutoSave()
{
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }
    m_autoSaveTimer->start(m_autoSaveInterval * 1000);
}



