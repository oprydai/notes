#include "MarkdownHighlighter.h"

#include <QBrush>
#include <QColor>
#include <QFont>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    
    // Bold **text** or __text__
    QTextCharFormat bold;
    bold.setFontWeight(QFont::Bold);
    bold.setForeground(QColor(255, 255, 255));
    m_rules.push_back({QRegExp("\\*\\*([^*]+)\\*\\*"), bold});
    m_rules.push_back({QRegExp("__([^_]+)__"), bold});

    // Italic *text* or _text_
    QTextCharFormat italic;
    italic.setFontItalic(true);
    italic.setForeground(QColor(230, 230, 230));
    m_rules.push_back({QRegExp("\\*([^*]+)\\*"), italic});
    m_rules.push_back({QRegExp("_([^_]+)_"), italic});

    // Bold and italic ***text*** or ___text___
    QTextCharFormat boldItalic;
    boldItalic.setFontWeight(QFont::Bold);
    boldItalic.setFontItalic(true);
    boldItalic.setForeground(QColor(255, 255, 255));
    m_rules.push_back({QRegExp("\\*\\*\\*([^*]+)\\*\\*\\*"), boldItalic});
    m_rules.push_back({QRegExp("___([^_]+)___"), boldItalic});

    // Strikethrough ~~text~~
    QTextCharFormat strikethrough;
    strikethrough.setFontStrikeOut(true);
    strikethrough.setForeground(QColor(150, 150, 150));
    m_rules.push_back({QRegExp("~~([^~]+)~~"), strikethrough});

    // Inline code `code`
    m_codeInline.setFontFamily("Consolas, Monaco, 'Courier New', monospace");
    m_codeInline.setBackground(QColor(40, 40, 40));
    m_codeInline.setForeground(QColor(220, 220, 220));
    m_codeInline.setFontWeight(QFont::Medium);
    m_rules.push_back({QRegExp("`([^`]+)`"), m_codeInline});

    // Links [text](url)
    m_link.setForeground(QColor(100, 180, 255));
    m_link.setFontUnderline(true);
    m_rules.push_back({QRegExp("\\[([^\\]]+)\\]\\(([^)]+)\\)"), m_link});

    // Images ![alt](url)
    QTextCharFormat image;
    image.setForeground(QColor(255, 193, 7));
    image.setFontItalic(true);
    m_rules.push_back({QRegExp("!\\[([^\\]]*)\\]\\(([^)]+)\\)"), image});

    // Checkbox - [ ] or - [x]
    m_checkbox.setForeground(QColor(100, 200, 100));
    m_checkbox.setFontWeight(QFont::Bold);
    m_rules.push_back({QRegExp("^- \\[([ xX])\\] "), m_checkbox});

    // Blockquotes > text
    QTextCharFormat blockquote;
    blockquote.setForeground(QColor(180, 180, 180));
    blockquote.setFontItalic(true);
    m_rules.push_back({QRegExp("^> (.+)"), blockquote});

    // Horizontal rules --- or ***
    QTextCharFormat hr;
    hr.setForeground(QColor(100, 100, 100));
    hr.setFontWeight(QFont::Bold);
    m_rules.push_back({QRegExp("^[\\*\\-]{3,}$"), hr});

    // Headings with better formatting
    m_heading1.setFontWeight(QFont::Bold);
    m_heading1.setFontPointSize(24);
    m_heading1.setForeground(QColor(255, 255, 255));
    m_heading1.setBackground(QColor(30, 30, 30));

    m_heading2.setFontWeight(QFont::DemiBold);
    m_heading2.setFontPointSize(20);
    m_heading2.setForeground(QColor(240, 240, 240));
    m_heading2.setBackground(QColor(25, 25, 25));
}

void MarkdownHighlighter::setActiveBlockNumber(int blockNumber) {
    m_activeBlockNumber = blockNumber;
}

void MarkdownHighlighter::highlightBlock(const QString &text) {
    // Headings with enhanced styling
    if (text.startsWith("# ")) {
        setFormat(0, text.length(), m_heading1);
    } else if (text.startsWith("## ")) {
        setFormat(0, text.length(), m_heading2);
    } else if (text.startsWith("### ")) {
        QTextCharFormat h3;
        h3.setFontWeight(QFont::DemiBold);
        h3.setFontPointSize(18);
        h3.setForeground(QColor(230, 230, 230));
        h3.setBackground(QColor(22, 22, 22));
        setFormat(0, text.length(), h3);
    } else if (text.startsWith("#### ")) {
        QTextCharFormat h4;
        h4.setFontWeight(QFont::Medium);
        h4.setFontPointSize(16);
        h4.setForeground(QColor(220, 220, 220));
        h4.setBackground(QColor(20, 20, 20));
        setFormat(0, text.length(), h4);
    } else if (text.startsWith("##### ")) {
        QTextCharFormat h5;
        h5.setFontWeight(QFont::Medium);
        h5.setFontPointSize(14);
        h5.setForeground(QColor(210, 210, 210));
        setFormat(0, text.length(), h5);
    } else if (text.startsWith("###### ")) {
        QTextCharFormat h6;
        h6.setFontWeight(QFont::Medium);
        h6.setFontPointSize(13);
        h6.setForeground(QColor(200, 200, 200));
        setFormat(0, text.length(), h6);
    }

    // Lists
    if (text.startsWith("- ") || text.startsWith("* ") || text.startsWith("+ ")) {
        QTextCharFormat list;
        list.setForeground(QColor(100, 200, 100));
        list.setFontWeight(QFont::Bold);
        setFormat(0, 2, list);
    }

    // Numbered lists
    QRegExp numberedList("^(\\d+)\\. ");
    if (numberedList.indexIn(text) == 0) {
        QTextCharFormat list;
        list.setForeground(QColor(100, 200, 100));
        list.setFontWeight(QFont::Bold);
        setFormat(0, numberedList.matchedLength(), list);
    }

    // Code blocks (fenced with ```)
    if (text.startsWith("```")) {
        QTextCharFormat codeBlock;
        codeBlock.setFontFamily("Consolas, Monaco, 'Courier New', monospace");
        codeBlock.setBackground(QColor(35, 35, 35));
        codeBlock.setForeground(QColor(220, 220, 220));
        codeBlock.setFontWeight(QFont::Medium);
        setFormat(0, text.length(), codeBlock);
    }

    // If this is the active editing block and it is a heading, make the hashes more subtle
    if (currentBlock().blockNumber() == m_activeBlockNumber) {
        if (text.startsWith("#")) {
            int hashCount = 0;
            while (hashCount < text.size() && text.at(hashCount) == '#') {
                ++hashCount;
            }
            if (hashCount > 0) {
                QTextCharFormat subtle;
                subtle.setForeground(QColor(80, 80, 80));
                setFormat(0, hashCount + 1, subtle);
            }
        }
    }

    // Apply inline rules
    for (const auto &rule : m_rules) {
        QRegExp rx(rule.pattern);
        int index = rx.indexIn(text);
        while (index >= 0) {
            const int length = rx.matchedLength();
            setFormat(index, length, rule.format);
            index = rx.indexIn(text, index + length);
        }
    }

    // Highlight current line (subtle background)
    if (currentBlock().blockNumber() == m_activeBlockNumber) {
        QTextCharFormat currentLine;
        currentLine.setBackground(QColor(30, 30, 30));
        setFormat(0, text.length(), currentLine);
    }
}


