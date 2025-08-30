#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QTextEdit>
#include <QTimer>

class TextEditor : public QTextEdit {
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    
    void setAutoSaveEnabled(bool enabled);
    void setAutoSaveInterval(int seconds);
    
signals:
    void contentChanged();
    void autoSaveRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void onTextChanged();
    void onAutoSaveTimeout();

private:
    void scheduleAutoSave();
    
    QTimer *m_autoSaveTimer;
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
};

#endif // TEXTEDITOR_H


