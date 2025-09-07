#ifndef GOOGLEAUTHDIALOG_H
#define GOOGLEAUTHDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>

class GoogleAuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoogleAuthDialog(QWidget *parent = nullptr);
    QString getAuthCode() const;

private slots:
    void openAuthUrl();
    void validateAuthCode();
    void onAuthCodeChanged();

private:
    void setupUI();
    void updateInstructions();
    void updateStatus();

    QLabel *m_titleLabel;
    QLabel *m_instructionsLabel;
    QLabel *m_statusLabel;  // New status label
    QTextEdit *m_authCodeEdit;
    QPushButton *m_openBrowserButton;
    QPushButton *m_validateButton;
    QPushButton *m_cancelButton;
    QProgressBar *m_progressBar;
    
    QString m_authUrl;
    QString m_authCode;
};

#endif // GOOGLEAUTHDIALOG_H
