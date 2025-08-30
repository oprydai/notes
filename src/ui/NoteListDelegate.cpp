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
    return {option.rect.width(), 80};
}

void NoteListDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    QStyleOptionViewItem option(opt);
    option.state &= ~QStyle::State_HasFocus;

    const QString title = idx.data(Qt::DisplayRole).toString();
    const QString snippet = idx.data(Roles::NoteSnippetRole).toString();
    const QDateTime date = idx.data(Roles::NoteDateRole).toDateTime();
    const bool pinned = idx.data(Roles::NotePinnedRole).toBool();

    // Create rounded rectangle for the note item
    QRect itemRect = option.rect.adjusted(4, 2, -4, -2);
    QPainterPath itemPath;
    itemPath.addRoundedRect(itemRect, 12, 12);

    // Background
    if (option.state & QStyle::State_Selected) {
        p->fillPath(itemPath, QColor(0, 122, 255, 40));
        p->setPen(QPen(QColor(0, 122, 255, 80), 1));
        p->drawPath(itemPath);
    } else if (option.state & QStyle::State_MouseOver) {
        p->fillPath(itemPath, QColor(255, 255, 255, 15));
    } else {
        p->fillPath(itemPath, QColor(255, 255, 255, 5));
    }

    // Content area
    QRect contentRect = itemRect.adjusted(16, 12, -16, -12);
    
    // Pin indicator
    if (pinned) {
        QPainterPath pinPath;
        QRect pinRect(contentRect.left(), contentRect.top(), 16, 16);
        pinPath.addEllipse(pinRect);
        p->fillPath(pinPath, QColor(255, 193, 7));
        p->setPen(QPen(QColor(255, 193, 7), 1));
        p->drawPath(pinPath);
        
        // Pin icon (simplified)
        p->setPen(QColor(0, 0, 0));
        p->setFont(QFont("Arial", 8, QFont::Bold));
        p->drawText(pinRect, Qt::AlignCenter, "📌");
        
        contentRect.setLeft(contentRect.left() + 24);
    }

    // Title
    QFont titleFont = option.font;
    if (titleFont.pointSize() <= 0) {
        titleFont = QFont("Arial", 12); // Default font if none set
    }
    int titleSize = qMax(1, titleFont.pointSize() + 1);
    titleFont.setPointSize(titleSize);
    titleFont.setWeight(QFont::DemiBold);
    p->setFont(titleFont);
    p->setPen(QColor(240, 240, 240));

    QRect titleRect(contentRect.left(), contentRect.top(), contentRect.width() - 80, 24);
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
    p->setPen(QColor(150, 150, 150));
    
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
    
    QRect dateRect(contentRect.right() - 80, contentRect.top(), 80, 24);
    p->drawText(dateRect, Qt::AlignRight | Qt::AlignVCenter, dateText);

    // No snippet text - only show title

    // Bottom border
    p->setPen(QPen(QColor(255, 255, 255, 20), 1));
    p->drawLine(itemRect.bottomLeft() + QPoint(16, -1), itemRect.bottomRight() + QPoint(-16, -1));

    p->restore();
}


