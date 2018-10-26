
#include <QHeaderView>
#include <QKeyEvent>
#include <QRegularExpression>

#include "CatchTableModel.h"
#include "CatchTableView.h"

CatchTableView::CatchTableView(QWidget* parent)
    : QTableView(parent)
{
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    setWordWrap(true);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setVisible(false);
}

void CatchTableView::keyPressEvent(QKeyEvent* event)
{
    // Handle value changes if the Ctrl key is pressed.
    if ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) && 
        event->modifiers().testFlag(Qt::ControlModifier))
    {
        QModelIndex idx = currentIndex();
        qobject_cast<CatchTableModel*>(model())->addOrSubtractCatch(idx, event->key() == Qt::Key_Up);
    }
    else
    {
        QTableView::keyPressEvent(event);
    }
}

void CatchTableView::resizeColumns()
{
    for (int i = 0; i < model()->columnCount(); ++i)
    {
        resizeColumn(i);
    }
}

void CatchTableView::resizeColumn(int col)
{
    if (col == 0)
    {
        resizeColumnToContents(0);
    }
    else
    {
        QModelIndex idx = qobject_cast<CatchTableModel*>(model())->index(0, col);
        QString text = idx.data(Qt::DisplayRole).toString();

        // If there's a "(Lv. #)" string at the end, append it to the last item.
        QRegularExpression regExp(".+\\(Lv\\. \\d+\\)");
        if (regExp.match(text).hasMatch())
        {
            // Split the item's text into individual words.
            QStringList strings = text.split(" ");
            QString lvlString2 = strings.takeLast();
            QString lvlString1 = strings.takeLast();
            strings.last() += (" " + lvlString1 + " " + lvlString2);

            int maxStrWidth = 0;
            for (auto str : strings)
            {
                maxStrWidth = qMax(maxStrWidth, fontMetrics().width(str));
            }

            setColumnWidth(col, maxStrWidth + 8);
        }
        else
        {
            resizeColumnToContents(col);
        }
    }
    resizeRowToContents(0);
}

void CatchTableView::wheelEvent(QWheelEvent* event)
{
    QModelIndex idx = indexAt(event->pos());
    CatchTableModel* catchModel = qobject_cast<CatchTableModel*>(model());
    if (!catchModel->addOrSubtractCatch(idx, event->angleDelta().ry() > 0))
    {
        event->ignore();
    }
}
