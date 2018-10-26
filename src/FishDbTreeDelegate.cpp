
#include <QSpinBox>

#include "FishDbTreeModel.h"

#include "FishDbTreeDelegate.h"

FishDbTreeDelegate::FishDbTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* FishDbTreeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() == FishDbTreeModel::LvlCol)
    {
        QSpinBox* editor = new QSpinBox(parent);
        editor->setMinimum(1);
        editor->setButtonSymbols(QSpinBox::NoButtons);
        return editor;
    }
    else
    {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void FishDbTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (index.column() == FishDbTreeModel::LvlCol)
    {
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
        spinBox->setValue(index.data(FishDbTreeModel::LvlRole).toInt());
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void FishDbTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (index.column() == FishDbTreeModel::LvlCol)
    {
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(editor);
        // itemChanged handling depends on the level role being set first.
        model->setData(index, QVariant::fromValue(spinBox->value()), FishDbTreeModel::LvlRole);
        model->setData(index, "Lv. " + QString::number(spinBox->value()), Qt::DisplayRole);
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}
