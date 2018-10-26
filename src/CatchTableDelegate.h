
#pragma once

#include <QStyledItemDelegate>

class CatchTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    CatchTableDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

signals:
    void baitNameChanged(const QModelIndex& index) const;
    void catchCountChanged(const QModelIndex& index) const;
    void fishNameChanged(const QModelIndex& index) const;
};
