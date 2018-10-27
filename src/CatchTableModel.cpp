
#include <QSqlError>
#include <QSqlQuery>

#include "FishDb.h"

#include "CatchTableModel.h"

CatchTableModel::CatchTableModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_currentSpotId("")
    , m_freshwater(1)
    , m_numFish(0)
{
    setSortRole(LevelRole);
}

void CatchTableModel::clear()
{
    m_currentSpotId = "";
    QStandardItemModel::clear();
}

void CatchTableModel::setSpot(QString spotId, bool forceReset)
{
    if (spotId == m_currentSpotId && !forceReset)
        return;

    clear();

    m_currentSpotId = spotId;

    QSqlQuery freshwaterQuery(FishDb::db());
    freshwaterQuery.exec("SELECT freshwater, num_fish FROM spots WHERE id = " + spotId);
    freshwaterQuery.next();
    m_freshwater = freshwaterQuery.value(0).toInt();
    m_numFish = freshwaterQuery.value(1).toInt();
    setColumnCount(m_numFish);

    QStringList headerLabels{ "Bait" };
    for (int i = 1; i <= m_numFish; ++i)
    {
        headerLabels.append("Fish " + QString::number(i));
    }
    setHorizontalHeaderLabels(headerLabels);

    // Build a list of all the fish for the first row.
    QStandardItem* emptyItem = new QStandardItem;
    emptyItem->setData(-99, LevelRole); // Trick for sorting to keep this row on top.
    emptyItem->setEditable(false);
    QList<QStandardItem*> fishItemList{ emptyItem };

    QSqlQuery fishQuery(FishDb::db());
    fishQuery.exec("SELECT id, name, level, sort_order FROM fish, spot_fish WHERE spot_id = " + spotId + " AND spot_fish.fish_id = fish.id ORDER BY sort_order");
    bool queryValid = fishQuery.next();
    for (int i = 1; i <= m_numFish; ++i)
    {
        // Default to an unknown fish.
        QStandardItem* fishItem = new QStandardItem("???");
        fishItem->setData(999, LevelRole);
        fishItem->setTextAlignment(Qt::AlignCenter);

        // If we have a known fish for this column, update the item.
        if (queryValid && fishQuery.value(3).toInt() == i)
        {
            QString name = fishQuery.value(1).toString();
            QString level = fishQuery.value(2).toString();
            fishItem->setText(name + " (Lv. " + level + ")");
            fishItem->setData(fishQuery.value(0), IdRole);
            fishItem->setData(name, NameRole);
            fishItem->setData(level.toInt(), LevelRole);

            queryValid = fishQuery.next();
        }

        fishItemList.append(fishItem);
    }
    appendRow(fishItemList);

    // Add a row for each bait with the same water type.
    QSqlQuery baitQuery(FishDb::db());
    baitQuery.exec("SELECT id, name, level FROM bait WHERE freshwater = " + QString::number(m_freshwater) + " ORDER BY level");
    while (baitQuery.next())
    {
        QString baitId = baitQuery.value(0).toString();
        QString name = baitQuery.value(1).toString();
        QString level = baitQuery.value(2).toString();
        QStandardItem* baitItem = new QStandardItem(name + " (Lv. " + level + ")");
        baitItem->setEditable(false);
        baitItem->setData(baitId, IdRole);
        baitItem->setData(name, NameRole);
        baitItem->setData(level.toInt(), LevelRole);

        QList<QStandardItem*> baitItems{baitItem};
        for (int i = 1; i <= m_numFish; ++i)
        {
            // Default to an uninteractable item if we don't know which fish this corresponds to.
            QStandardItem* catchItem = new QStandardItem("-");

            QVariant fishIdVar = fishItemList.at(i)->data(IdRole);
            if (fishIdVar.isValid())
            {
                QSqlQuery catchQuery(FishDb::db());
                catchQuery.exec("SELECT count FROM catches WHERE bait_id = " + baitId +
                                " AND spot_id = " + m_currentSpotId +
                                " AND fish_id = " + fishIdVar.toString());
                if (catchQuery.next())
                    catchItem->setData(catchQuery.value(0), CatchRole);
            }

            catchItem->setTextAlignment(Qt::AlignCenter);
            catchItem->setEditable(false);
            baitItems.append(catchItem);
        }

        appendRow(baitItems);
        updateCatches(rowCount() - 1);
    }

    // Add a row to add a new bait.
    appendBaitRow();
}

void CatchTableModel::setBaitLevel(const QModelIndex& index, int level)
{
    QStandardItem* item = itemFromIndex(index);
    item->setData(level, LevelRole);
    item->setText(item->data(NameRole).toString() + " (Lv. " + QString::number(level) + ")");

    QSqlQuery query(FishDb::db());
    query.exec("UPDATE bait SET level = " + QString::number(level) + " WHERE id = " + item->data(IdRole).toString());

    sort(0);
}

void CatchTableModel::setFishLevel(const QModelIndex& index, int level)
{
    QStandardItem* item = itemFromIndex(index);
    item->setData(level, LevelRole);
    item->setText(item->data(NameRole).toString() + " (Lv. " + QString::number(level) + ")");

    QSqlQuery query(FishDb::db());
    query.exec("UPDATE fish SET level = " + QString::number(level) + " WHERE id = " + item->data(IdRole).toString());
}

bool CatchTableModel::addOrSubtractCatch(const QModelIndex& index, bool add)
{
    // Do nothing if we're in the first column or the first/last rows.
    if (index.column() == 0 || index.row() == 0 || index.row() == rowCount() - 1)
    {
        return false;
    }

    // Also do nothing if this column is for an unknown fish.
    QModelIndex fishIdx = index.sibling(0, index.column());
    if (!fishIdx.data(IdRole).isValid())
    {
        return false;
    }

    int catches = index.data(CatchRole).toInt();
    catches += add ? 1 : -1;
    setData(index, qMax(catches, 0), CatchRole);

    QString baitId = item(index.row(), 0)->data(IdRole).toString();
    QString fishId = item(0, index.column())->data(IdRole).toString();
    QSqlQuery query(FishDb::db());
    query.exec("REPLACE INTO catches (spot_id, bait_id, fish_id, count) VALUES (" +
        m_currentSpotId + ", " + baitId + ", " + fishId + ", " + index.data(CatchRole).toString() + ")");

    updateCatches(index.row());

    return true;
}

void CatchTableModel::handleBaitNameChanged(const QModelIndex& index)
{
    QStandardItem* changedItem = itemFromIndex(index);
    QFont font = changedItem->font();
    font.setItalic(false);
    changedItem->setFont(font);
    changedItem->setForeground(QColor(0, 0, 0));

    QString name = index.data(NameRole).toString();
    int lvl = index.data(LevelRole).toInt();
    QSqlQuery query(FishDb::db());

    // Add the bait if it doesn't exist yet.
    if (!index.data(IdRole).isValid())
    {
        lvl = 1;
        changedItem->setData(1, LevelRole);
        query.exec("INSERT INTO bait (name, level, freshwater) VALUES (\"" + name + "\", 1, " + QString::number(m_freshwater) + ")");

        query.exec("SELECT id FROM bait WHERE name = \"" + name + "\"");
        query.next();
        QString baitId = query.value(0).toString();
        changedItem->setData(baitId, IdRole);

        // Update catch items for known fish for this bait.
        for (int col = 1; col <= m_numFish; ++col)
        {
            QStandardItem* fishItem = item(0, col);
            if (!fishItem->data(IdRole).isValid())
            {
                item(changedItem->row(), col)->setText("-");
            }
            else
            {
                item(changedItem->row(), col)->setText("0");
                item(changedItem->row(), col)->setEditable(true);
            }
        }

        appendBaitRow();
    }
    // Otherwise, just update the existing bait. (With set items being unchangeable, this shouldn't happen.)
    else
    {
        query.exec("UPDATE bait SET name = \"" + name + "\" WHERE id = " + index.data(IdRole).toString());
    }

    changedItem->setText(name + " (Lv. " + QString::number(lvl) + ")");
}

void CatchTableModel::handleFishNameChanged(const QModelIndex& index)
{
    QString name = index.data(NameRole).toString();

    QString fishId;
    int lvl = 1;

    // First, try to grab the id and level for the fish.
    QSqlQuery query(FishDb::db());
    query.exec("SELECT id, level FROM fish WHERE name = \"" + name + "\"");
    // If there's an existing fish with this name, grab the id and level.
    if (query.next())
    {
        fishId = query.value(0).toString();
        lvl = query.value(1).toInt();
    }
    // Otherwise, add a new fish entry and grab its id.
    else
    {
        query.exec("INSERT INTO FISH (name, level) VALUES (\"" + name + "\", 1)");
        query.exec("SELECT id FROM fish WHERE name = \"" + name + "\"");
        query.next();
        fishId = query.value(0).toString();
    }

    // If there already was a column for this fish, set it to an unknown fish.
    query.exec("SELECT sort_order FROM spot_fish WHERE "
        "spot_id = " + m_currentSpotId + " AND fish_id = " + fishId);
    if (query.next())
    {
        int col = query.value(0).toInt();
        item(0, col)->setText("???");
        item(0, col)->setData(QVariant(), NameRole);
        item(0, col)->setData(QVariant(), IdRole);
        item(0, col)->setData(1, LevelRole);

        for (int row = 1; row < rowCount() - 1; ++row)
        {
            item(row, col)->setData(QVariant(), CatchRole);
        }

        query.exec("DELETE FROM spot_fish WHERE spot_id = " + m_currentSpotId + " AND sort_order = " + QString::number(col));
    }

    QStandardItem* changedItem = itemFromIndex(index);
    changedItem->setData(fishId, IdRole);
    changedItem->setData(lvl, LevelRole);

    query.exec("REPLACE INTO spot_fish (spot_id, fish_id, sort_order) VALUES (" +
               m_currentSpotId + ", " + fishId + ", " + QString::number(index.column()) + ")");

    // Update catch items for this fish for all bait.
    for (int row = 1; row < rowCount() - 1; ++row)
    {
        QString baitId = item(row, 0)->data(IdRole).toString();
        query.exec("SELECT count FROM catches WHERE bait_id = " + baitId + " AND spot_id = " + m_currentSpotId + " AND fish_id = " + fishId);
        int catchCount = 0;
        if (query.next())
            catchCount = query.value(0).toInt();

        item(row, index.column())->setData(catchCount, CatchRole);
        updateCatches(row);
    }

    changedItem->setText(name + " (Lv. " + QString::number(lvl) + ")");
}

void CatchTableModel::updateCatches(int row)
{
    int totalCatches = 0;
    for (int col = 1; col < columnCount(); ++col)
    {
        totalCatches += index(row, col).data(CatchRole).toInt();
    }

    for (int i = 1; i < columnCount(); ++i)
    {
        QVariant fishIdVar = index(0, i).data(IdRole);
        if (fishIdVar.isValid())
        {
            int catches = index(row, i).data(CatchRole).toInt();
            QString text = QString::number(catches);
            if (catches > 0 && totalCatches > 0)
            {
                int percent = static_cast<int>(std::round(float(catches * 100) / float(totalCatches)));
                text += (" (" + QString::number(percent) + "%)");
            }
            item(row, i)->setText(text);
        }
        else
        {
            item(row, i)->setText("-");
        }
    }
}

void CatchTableModel::appendBaitRow()
{
    // Add a row at the bottom to add a new bait.
    QStandardItem* addBaitItem = new QStandardItem("Add new bait...");
    QFont itemFont = addBaitItem->font();
    itemFont.setItalic(true);
    addBaitItem->setFont(itemFont);
    addBaitItem->setForeground(QColor(150, 150, 150));
    addBaitItem->setData(999, LevelRole); // Trick for sorting to keep this on the bottom.

    QList<QStandardItem*> items{addBaitItem};
    for (int i = 1; i <= m_numFish; ++i)
    {
        QStandardItem* item = new QStandardItem;
        item->setTextAlignment(Qt::AlignCenter);
        item->setEditable(false);
        items.append(item);
    }

    appendRow(items);
    sort(0);
}
