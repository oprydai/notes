#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class MarkdownHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);
    void setActiveBlockNumber(int blockNumber);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule { QRegExp pattern; QTextCharFormat format; };
    QVector<Rule> m_rules;
    QTextCharFormat m_heading1;
    QTextCharFormat m_heading2;
    QTextCharFormat m_codeInline;
    QTextCharFormat m_link;
    QTextCharFormat m_checkbox;
    int m_activeBlockNumber = -1;
};


