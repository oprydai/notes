#include "GoogleAuthDialog.h"
#include "../sync/ConfigLoader.h"
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QStyle>
#include <QTimer>

GoogleAuthDialog::GoogleAuthDialog(QWidget *parent)
    : QDialog(parent)
    , m_authUrl("")
{
    setWindowTitle("Connect to Google Drive");
    setModal(true);
    setFixedSize(500, 400);
    
    setupUI();
    updateInstructions();
    updateStatus();
}

void GoogleAuthDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("Connect to Google Drive");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);
    
    // Instructions
    m_instructionsLabel = new QLabel();
    m_instructionsLabel->setWordWrap(true);
    m_instructionsLabel->setStyleSheet("margin: 10px; padding: 10px; background-color: #f0f0f0; border-radius: 5px;");
    mainLayout->addWidget(m_instructionsLabel);
    
    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("margin: 5px; padding: 8px; background-color: #e8f5e8; border: 1px solid #34a853; border-radius: 3px; color: #2d8f47; font-size: 11px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    // Open browser button
    m_openBrowserButton = new QPushButton("Open Google Sign-in Page");
    m_openBrowserButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #4285f4; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #3367d6; }");
    connect(m_openBrowserButton, &QPushButton::clicked, this, &GoogleAuthDialog::openAuthUrl);
    mainLayout->addWidget(m_openBrowserButton);
    
    // Auth code input
    QLabel *codeLabel = new QLabel("Authorization Code:");
    mainLayout->addWidget(codeLabel);
    
    m_authCodeEdit = new QTextEdit();
    m_authCodeEdit->setMaximumHeight(80);
    m_authCodeEdit->setPlaceholderText("Paste the authorization code here...");
    connect(m_authCodeEdit, &QTextEdit::textChanged, this, &GoogleAuthDialog::onAuthCodeChanged);
    mainLayout->addWidget(m_authCodeEdit);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_validateButton = new QPushButton("Connect");
    m_validateButton->setEnabled(false);
    m_validateButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #34a853; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #2d8f47; } QPushButton:disabled { background-color: #ccc; }");
    connect(m_validateButton, &QPushButton::clicked, this, &GoogleAuthDialog::validateAuthCode);
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color:rgba(234, 68, 53, 0.91); color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #d33426; }");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(m_validateButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Set layout
    setLayout(mainLayout);
}

void GoogleAuthDialog::updateInstructions()
{
    QString clientId = ConfigLoader::instance().getClientId();
    QString instructions = 
        "<b>Step 1:</b> Click the button below to open Google's sign-in page.<br><br>"
        "<b>Step 2:</b> Sign in with your Google account and grant permission to access Google Drive.<br><br>"
        "<b>Step 3:</b> Copy the authorization code that appears on the page.<br><br>"
        "<b>Step 4:</b> Paste the code in the text box below and click 'Connect'.<br><br>"
        "<b>Note:</b> Your app is configured with Client ID: " + clientId.mid(0, 20) + "...";
    
    m_instructionsLabel->setText(instructions);
}

void GoogleAuthDialog::updateStatus()
{
    QString clientId = ConfigLoader::instance().getClientId();
    QString clientSecret = ConfigLoader::instance().getClientSecret();
    
    if (!clientId.isEmpty() && !clientSecret.isEmpty()) {
        m_statusLabel->setText("✓ Configuration loaded successfully - Ready to connect");
        m_statusLabel->setStyleSheet("margin: 5px; padding: 8px; background-color: #e8f5e8; border: 1px solid #34a853; border-radius: 3px; color: #2d8f47; font-size: 11px;");
    } else {
        m_statusLabel->setText("⚠ Configuration incomplete - Check console for details");
        m_statusLabel->setStyleSheet("margin: 5px; padding: 8px; background-color: #fff3cd; border: 1px solid #ffc107; border-radius: 3px; color: #856404; font-size: 11px;");
    }
}

void GoogleAuthDialog::openAuthUrl()
{
    // Load client ID from ConfigLoader for security
    QString clientId = ConfigLoader::instance().getClientId();
    
    if (clientId.isEmpty()) {
        QMessageBox::critical(this, "Configuration Error", 
            "Google Drive API credentials not found!\n\n"
            "Please ensure you have a valid config file or environment variables set.\n"
            "Check the console for configuration details.");
        return;
    }
    
    // Build the OAuth URL with proper encoding
    // Use the correct Google OAuth 2.0 endpoint for desktop apps
    QUrl url("https://accounts.google.com/o/oauth2/auth");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("redirect_uri", "urn:ietf:wg:oauth:2.0:oob");
    query.addQueryItem("scope", "https://www.googleapis.com/auth/drive.file");
    query.addQueryItem("response_type", "code");
    query.addQueryItem("access_type", "offline");
    query.addQueryItem("prompt", "consent");
    
    url.setQuery(query);
    QString authUrl = url.toString();
    
    m_authUrl = authUrl;
    
    // Show the URL in a message box for user reference
    QMessageBox::information(this, "Google OAuth URL", 
        "Opening Google's authorization page in your browser.\n\n"
        "If the browser doesn't open automatically, you can manually copy and paste this URL:\n\n" + authUrl + "\n\n"
        "If you get a 404 error, please check:\n"
        "1. Your Google Cloud Console OAuth consent screen is configured\n"
        "2. The OAuth 2.0 client ID is active\n"
        "3. The redirect URI is added to your OAuth client");
    
    // Open the URL in the default browser
    if (QDesktopServices::openUrl(QUrl(authUrl))) {
        m_openBrowserButton->setText("✓ Browser Opened");
        m_openBrowserButton->setEnabled(false);
        m_openBrowserButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; background-color: #34a853; color: white; border: none; border-radius: 5px; }");
    } else {
        QMessageBox::warning(this, "Browser Error", 
            "Failed to open browser automatically.\n\n"
            "Please manually navigate to:\n\n" + authUrl);
    }
}

void GoogleAuthDialog::onAuthCodeChanged()
{
    QString text = m_authCodeEdit->toPlainText().trimmed();
    m_validateButton->setEnabled(!text.isEmpty());
}

void GoogleAuthDialog::validateAuthCode()
{
    m_authCode = m_authCodeEdit->toPlainText().trimmed();
    
    if (m_authCode.isEmpty()) {
        QMessageBox::warning(this, "Invalid Code", "Please enter the authorization code from Google's page.");
        return;
    }
    
    // Validate the format (Google auth codes are typically 4/... format)
    if (!m_authCode.startsWith("4/")) {
        QMessageBox::warning(this, "Invalid Code Format", 
            "The authorization code doesn't appear to be in the correct format.\n\n"
            "Google authorization codes typically start with '4/'.\n"
            "Please make sure you copied the entire code from Google's page.");
        return;
    }
    
    // Complete the OAuth flow immediately
    accept();
}

QString GoogleAuthDialog::getAuthCode() const
{
    return m_authCode;
}
