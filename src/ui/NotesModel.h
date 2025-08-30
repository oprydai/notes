#pragma once

#include <QStandardItemModel>
#include <QMimeData>
#include <QDataStream>

class NotesModel : public QStandardItemModel {
    Q_OBJECT

public:
    explicit NotesModel(QObject *parent = nullptr);

    // Drag and drop support
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
};
