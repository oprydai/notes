#include "NotesModel.h"

NotesModel::NotesModel(QObject *parent)
    : QStandardItemModel(parent) {
}

Qt::ItemFlags NotesModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
    
    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsDragEnabled;
    }
    
    return defaultFlags;
}

QStringList NotesModel::mimeTypes() const {
    QStringList types;
    types << "application/x-notes-item";
    return types;
}

QMimeData *NotesModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = new QMimeData();
    QByteArray itemData;
    QDataStream stream(&itemData, QIODevice::WriteOnly);
    
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && index.column() == 0) {
            int noteId = index.data(Qt::UserRole).toInt();
            stream << noteId;
            break; // Only drag one item at a time
        }
    }
    
    mimeData->setData("application/x-notes-item", itemData);
    return mimeData;
}
