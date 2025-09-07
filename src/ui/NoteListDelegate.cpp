#include "NoteListDelegate.h"
#include "../utils/Roles.h"

#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QTextLayout>
#include <QPainterPath>
#include <QFontMetrics>

NoteListDelegate::NoteListDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

QSize NoteListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    Q_UNUSED(index);
    return {option.rect.width(), 100};
}

void NoteListDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    QStyleOptionViewItem option(opt);
    option.state &= ~QStyle::State_HasFocus;

    const QString title = idx.data(Qt::DisplayRole).toString();
    const QString snippet = idx.data(Roles::NoteSnippetRole).toString();
    const QDateTime date = idx.data(Roles::NoteDateRole).toDateTime();


    // Create rounded rectangle for the note item
    QRect itemRect = option.rect.adjusted(6, 3, -6, -3);
    QPainterPath itemPath;
    itemPath.addRoundedRect(itemRect, 14, 14);

    // Background with gradient
    if (option.state & QStyle::State_Selected) {
        QLinearGradient selectedGradient(itemRect.topLeft(), itemRect.bottomLeft());
        selectedGradient.setColorAt(0, QColor(0, 122, 255, 50));
        selectedGradient.setColorAt(1, QColor(0, 122, 255, 30));
        p->fillPath(itemPath, selectedGradient);
        p->setPen(QPen(QColor(0, 122, 255, 100), 1.5));
        p->drawPath(itemPath);
    } else if (option.state & QStyle::State_MouseOver) {
        QLinearGradient hoverGradient(itemRect.topLeft(), itemRect.bottomLeft());
        hoverGradient.setColorAt(0, QColor(255, 255, 255, 20));
        hoverGradient.setColorAt(1, QColor(255, 255, 255, 8));
        p->fillPath(itemPath, hoverGradient);
    } else {
        QLinearGradient normalGradient(itemRect.topLeft(), itemRect.bottomLeft());
        normalGradient.setColorAt(0, QColor(255, 255, 255, 8));
        normalGradient.setColorAt(1, QColor(255, 255, 255, 3));
        p->fillPath(itemPath, normalGradient);
    }

    // Content area
    QRect contentRect = itemRect.adjusted(20, 16, -20, -16);
    
    // Title
    QFont titleFont = option.font;
    if (titleFont.pointSize() <= 0) {
        titleFont = QFont("Arial", 13); // Default font if none set
    }
    int titleSize = qMax(1, titleFont.pointSize() + 1);
    titleFont.setPointSize(titleSize);
    titleFont.setWeight(QFont::DemiBold);
    p->setFont(titleFont);
    p->setPen(QColor(245, 245, 245));

    QRect titleRect(contentRect.left(), contentRect.top(), contentRect.width() - 100, 28);
    QString elidedTitle = QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width());
    p->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

    // Date
    QFont dateFont = option.font;
    if (dateFont.pointSize() <= 0) {
        dateFont = QFont("Arial", 10); // Default font if none set
    }
    int dateSize = qMax(1, dateFont.pointSize() - 1);
    dateFont.setPointSize(dateSize);
    p->setFont(dateFont);
    p->setPen(QColor(160, 160, 160));
    
    QString dateText;
    if (date.isValid()) {
        QDate currentDate = QDate::currentDate();
        QDate noteDate = date.date();
        
        if (noteDate == currentDate) {
            dateText = "Today";
        } else if (noteDate == currentDate.addDays(-1)) {
            dateText = "Yesterday";
        } else if (noteDate.year() == currentDate.year()) {
            dateText = noteDate.toString("MMM d");
        } else {
            dateText = noteDate.toString("MMM d, yyyy");
        }
    }
    
    QRect dateRect(contentRect.right() - 100, contentRect.top(), 100, 28);
    p->drawText(dateRect, Qt::AlignRight | Qt::AlignVCenter, dateText);

    // Snippet text
    if (!snippet.isEmpty()) {
        QFont snippetFont = option.font;
        if (snippetFont.pointSize() <= 0) {
            snippetFont = QFont("Arial", 11); // Default font if none set
        }
        int snippetSize = qMax(1, snippetFont.pointSize() - 1);
        snippetFont.setPointSize(snippetSize);
        snippetFont.setWeight(QFont::Normal);
        p->setFont(snippetFont);
        p->setPen(QColor(180, 180, 180));
        
        QRect snippetRect(contentRect.left(), contentRect.top() + 32, contentRect.width(), 20);
        QString elidedSnippet = QFontMetrics(snippetFont).elidedText(snippet, Qt::ElideRight, snippetRect.width());
        p->drawText(snippetRect, Qt::AlignLeft | Qt::AlignVCenter, elidedSnippet);
    }

    // Subtle bottom border
    p->setPen(QPen(QColor(255, 255, 255, 15), 0.5));
    p->drawLine(itemRect.bottomLeft() + QPoint(20, -1), itemRect.bottomRight() + QPoint(-20, -1));

    p->restore();
}


