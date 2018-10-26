
#pragma once

#include <QStyledItemDelegate>

class FishDbTreeDelegate : public QStyledItemDelegate
{
public:    
    FishDbTreeDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};
