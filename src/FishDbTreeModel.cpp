
#include <iostream>

#include <QSqlError>
#include <QSqlQuery>
#include <QTreeWidget>

#include "FishDb.h"
#include "FishDbTreeModel.h"

FishDbTreeModel::FishDbTreeModel(QObject* parent)
    : QStandardItemModel(parent)
{
    setColumnCount(2);

    buildTree();

    connect(this, &QStandardItemModel::dataChanged, this, &FishDbTreeModel::handleDataChanged);
}

void FishDbTreeModel::buildTree()
{
    clear();

    QSqlQuery regionQuery(FishDb::db());
    regionQuery.exec("SELECT id, name FROM regions ORDER BY sort_order");
    while (regionQuery.next())
    {
        QStandardItem* regionItem = new QStandardItem(regionQuery.value(1).toString());
        regionItem->setData(regionQuery.value(0));
        QStandardItem* emptyRegionItem = new QStandardItem;
        emptyRegionItem->setEditable(false);
        appendRow(QList<QStandardItem*>{regionItem, emptyRegionItem});

        QSqlQuery zoneQuery(FishDb::db());
        zoneQuery.exec("SELECT id, name FROM zones WHERE region_id = " + regionQuery.value(0).toString() + " ORDER BY sort_order");
        while (zoneQuery.next())
        {
            QStandardItem* zoneItem = new QStandardItem(zoneQuery.value(1).toString());
            zoneItem->setData(zoneQuery.value(0));
            QStandardItem* emptyZoneItem = new QStandardItem;
            emptyZoneItem->setEditable(false);
            regionItem->appendRow(QList<QStandardItem*>{zoneItem, emptyZoneItem});

            QSqlQuery spotQuery(FishDb::db());
            spotQuery.exec("SELECT id, name, level, freshwater FROM spots WHERE zone_id = " + zoneQuery.value(0).toString() + " ORDER BY sort_order");
            while (spotQuery.next())
            {
                QStandardItem* nameItem = new QStandardItem(spotQuery.value(1).toString());
                nameItem->setData(spotQuery.value(0));
                nameItem->setData(spotQuery.value(3), FreshwaterRole);
                QStandardItem* lvlItem = new QStandardItem("Lv. " + spotQuery.value(2).toString());
                lvlItem->setData(QVariant::fromValue(spotQuery.value(2).toInt()));
                zoneItem->appendRow(QList<QStandardItem*>{nameItem, lvlItem});
            }
        }
    }
}

QModelIndex FishDbTreeModel::addRegion(QString name)
{
    QSqlQuery query(FishDb::db());
    bool ok = query.exec("INSERT INTO regions (name, sort_order) VALUES (\"" + name + "\", " + QString::number(rowCount()) + ")");

    if (!ok)
    {
        QString error = query.lastError().text();
        return QModelIndex();
    }

    query.exec("SELECT id FROM regions WHERE name IS \"" + name + "\" AND sort_order IS " + QString::number(rowCount()));
    if (query.next())
    {
        QStandardItem* item = new QStandardItem(name);
        item->setData(query.value(0), IdRole);
        QStandardItem* emptyItem = new QStandardItem;
        emptyItem->setEditable(false);
        appendRow(QList<QStandardItem*>{item, emptyItem});
        return item->index();
    }

    return QModelIndex();
}

QModelIndex FishDbTreeModel::addZone(const QModelIndex& regionIdx, QString name)
{
    QStandardItem* regionItem = itemFromIndex(regionIdx);
    QString regionId = regionItem->data(IdRole).toString();
    QSqlQuery query(FishDb::db());
    bool ok = query.exec("INSERT INTO zones (region_id, name, sort_order) VALUES (" +
        regionId + ", \"" + name + "\", " + QString::number(regionItem->rowCount()) + ")");

    if (!ok)
    {
        QString error = query.lastError().text();
        return QModelIndex();
    }

    query.exec("SELECT id FROM zones WHERE name IS \"" + name + "\" AND sort_order IS " + QString::number(regionItem->rowCount()));
    if (query.next())
    {
        QStandardItem* item = new QStandardItem(name);
        item->setData(query.value(0), IdRole);
        QStandardItem* emptyItem = new QStandardItem;
        emptyItem->setEditable(false);
        regionItem->appendRow(QList<QStandardItem*>{item, emptyItem});

        return item->index();
    }

    return QModelIndex();
}

QModelIndex FishDbTreeModel::addSpot(const QModelIndex& zoneIdx, QString name, int level, bool freshwater)
{
    QStandardItem* zoneItem = itemFromIndex(zoneIdx);
    QString zoneId = zoneItem->data(IdRole).toString();
    QString sortOrder = QString::number(zoneItem->rowCount());
    QSqlQuery query(FishDb::db());
    bool ok = query.exec("INSERT INTO spots (zone_id, name, level, freshwater, sort_order, num_fish) "
                         "VALUES (" + zoneId + ", \"" + name + "\", " + QString::number(level) + ", " +
                         QString::number(freshwater ? 1 : 0) + ", " + sortOrder + ", 9)");

    if (!ok)
    {
        QString error = query.lastError().text();
        return QModelIndex();
    }

    query.exec("SELECT id FROM spots WHERE name = \"" + name + "\" AND sort_order = " + sortOrder + " AND zone_id = " + zoneId);
    if (query.next())
    {
        QStandardItem* nameItem = new QStandardItem(name);
        nameItem->setData(query.value(0), IdRole);
        nameItem->setData(QVariant::fromValue(freshwater ? 1 : 0), FreshwaterRole);
        QStandardItem* lvlItem = new QStandardItem("Lv. " + QString::number(level));
        lvlItem->setData(QVariant::fromValue(level));
        zoneItem->appendRow(QList<QStandardItem*>{nameItem, lvlItem});

        return nameItem->index();
    }

    return QModelIndex();
}

void FishDbTreeModel::moveUp(const QModelIndex& idx)
{
    QStandardItem* previousItem = itemFromIndex(idx.sibling(idx.row() - 1, idx.column()));

    // Default the parent item to the invisible root.
    QStandardItem* parentItem = invisibleRootItem();

    // If the item above the current item has a parent, use that as the parent item.
    if (previousItem->parent())
    {
        parentItem = previousItem->parent();
    }

    QList<QStandardItem*> items = parentItem->takeRow(idx.row());
    parentItem->insertRow(previousItem->row(), items);

    QString table = getTableFromItem(items[0]);

    QSqlQuery query(FishDb::db());
    query.exec("UPDATE " + table + " SET sort_order = " + QString::number(items[0]->row()) +
        " WHERE id = " + items[0]->data(IdRole).toString());
    query.exec("UPDATE " + table + " SET sort_order = " + QString::number(previousItem->row()) +
        " WHERE id = " + previousItem->data(IdRole).toString());
}

void FishDbTreeModel::moveDown(const QModelIndex& idx)
{
    QStandardItem* nextItem = itemFromIndex(idx.sibling(idx.row() + 1, idx.column()));

    // Default the parent item to the invisible root.
    QStandardItem* parentItem = invisibleRootItem();

    // If the item below the current item has a parent, use that as the parent item.
    if (nextItem->parent())
    {
        parentItem = nextItem->parent();
    }

    QList<QStandardItem*> items = parentItem->takeRow(idx.row());
    parentItem->insertRow(nextItem->row() + 1, items);

    QString table = getTableFromItem(items[0]);

    QSqlQuery query(FishDb::db());
    query.exec("UPDATE " + table + " SET sort_order = " + QString::number(items[0]->row()) +
        " WHERE id = " + items[0]->data(IdRole).toString());
    query.exec("UPDATE " + table + " SET sort_order = " + QString::number(nextItem->row()) +
        " WHERE id = " + nextItem->data(IdRole).toString());
}

void FishDbTreeModel::setFreshwater(const QModelIndex& idx, bool isFreshwater)
{
    int val = isFreshwater ? 1 : 0;
    itemFromIndex(idx)->setData(QVariant::fromValue(val), FreshwaterRole);
    QSqlQuery(FishDb::db()).exec("UPDATE spots SET freshwater = " + QString::number(val) +
        " WHERE id = " + itemFromIndex(idx)->data(IdRole).toString());
}

void FishDbTreeModel::handleDataChanged(const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles)
{
    if(roles.size() > 0 && roles.indexOf(Qt::DisplayRole) < 0)
        return;

    QStandardItem* item = itemFromIndex(topLeft);

    if (item->column() == NameCol)
    {
        QString table = getTableFromItem(item);

        QSqlQuery query(FishDb::db());
        query.exec("UPDATE " + table + " SET name = \"" + item->text() + "\" "
            "WHERE id = " + item->data(IdRole).toString());

        if (table == "spots")
            emit spotChanged();
    }
    else if (item->column() == LvlCol)
    {
        QString id = itemFromIndex(item->index().sibling(item->row(), 0))->data(IdRole).toString();
        QString test = item->text();

        QSqlQuery query(FishDb::db());
        QString txt = "UPDATE spots SET level = " + item->data(LvlRole).toString() + " WHERE id = " + id;
        bool ok = query.exec("UPDATE spots SET level = " + item->data(LvlRole).toString() + " WHERE id = " + id);

        if (!ok)
        {
            std::cout << query.lastError().text().toStdString() << "\n";
        }

        emit spotChanged();
    }
}

QString FishDbTreeModel::getTableFromItem(const QStandardItem* item) const
{
    // Default to spot
    QString table = "spots";
    // No parent = region
    if (!item->parent())
    {
        table = "regions";
    }
    // One parent = zone
    else if (!item->parent()->parent())
    {
        table = "zones";
    }
    return table;
}
