
#include <QCompleter>
#include <QLineEdit>
#include <QSpinBox>
#include <QSqlQuery>

#include "CatchTableModel.h"
#include "FishDb.h"

#include "CatchTableDelegate.h"

CatchTableDelegate::CatchTableDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* CatchTableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // If this is the last row and not the first column, disable editing.
    if (!index.sibling(index.row() + 1, index.column()).isValid() && index.column() > 0)
        return nullptr;
    // If this is the first row, we're changing a fish name.
    else if (index.row() == 0)
    {
        QStringList fishNames;

        QSqlQuery query(FishDb::db());
        query.exec("SELECT name FROM fish WHERE name IS NOT \"" +
            index.data(CatchTableModel::NameRole).toString() + "\"");
        while (query.next())
        {
            fishNames.append(query.value(0).toString());
        }

        QLineEdit* editor = new QLineEdit(parent);
        QCompleter* completer = new QCompleter(fishNames);
        completer->setCompletionMode(QCompleter::InlineCompletion);
        editor->setCompleter(completer);
        return editor;
    }
    // Otherwise, if we're not in the first column, we're changing a catch count.
    else if (index.column() > 0)
    {
        // Only allow editing via Ctrl + Up/Down for now...
        return nullptr;

        // QSpinBox* editor = new QSpinBox(parent);
        // editor->setAlignment(Qt::AlignCenter);
        // editor->setMinimum(0);
        // return editor;
    }
    // Otherwise, we're changing a bait name.
    else
    {
        QStringList baitNames;

        QSqlQuery query(FishDb::db());
        query.exec("SELECT name FROM bait WHERE freshwater = " + index.data(CatchTableModel::FreshwaterRole).toString());
        while (query.next())
        {
            baitNames.append(query.value(0).toString());
        }

        QLineEdit* editor = new QLineEdit(parent);
        QCompleter* completer = new QCompleter(baitNames);
        completer->setCompletionMode(QCompleter::InlineCompletion);
        editor->setCompleter(completer);
        return editor;
    }
}

void CatchTableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    // If we're in the first row or column, we need to pull the fish/bait name from the NameRole.
    if (index.column() == 0 || index.row() == 0)
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        lineEdit->setText(index.data(CatchTableModel::NameRole).toString());
    }
    // Otherwise, do default handling for the editor.
    else
        QStyledItemDelegate::setEditorData(editor, index);
}

void CatchTableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    // If we're in the first row or column, just set the NameRole and let the model do further handling.
    if (index.column() == 0 || index.row() == 0)
    {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        QString text = lineEdit->text();

        // Split on a comma to handle levels (i.e., "Bait/Fish,[lvl]")
        QStringList strings = text.split(",");
        QString name = strings.at(0).trimmed();

        // If we haven't entered anything or the name matches the existing entry, just return.
        if (text.isEmpty() || name == index.data(CatchTableModel::NameRole).toString())
            return;

        int lvl = model->data(index, CatchTableModel::LevelRole).toInt(); // Default the level to whatever was in the data.
        // If there was a string after a comma, try to extract the level.
        if (strings.size() == 2)
        {
            bool ok;
            int newLvl = strings.at(1).trimmed().toInt(&ok);
            if (ok && newLvl > 0 && newLvl <= 70)
                lvl = newLvl;
        }

        // If we're in column 0, see if there's an existing bait to pull the level from.
        if (index.column() == 0)
        {
            QSqlQuery query(FishDb::db());
            query.exec("SELECT level FROM bait WHERE name = \"" + name + "\"");
            if (query.next())
            {
                lvl = query.value(0).toInt();
            }
        }

        model->setData(index, name, CatchTableModel::NameRole);
        model->setData(index, lvl, CatchTableModel::LevelRole);
        if (index.column() == 0)
            emit baitChanged(index);
        else
            emit fishChanged(index);
    }
    // Otherwise, don't do anything if we're in the last row.
    else if (index.row() == model->rowCount() - 1)
        return;
    // Otherwise, do default handling.
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}
