
#pragma once

#include <QStandardItemModel>

class FishDbTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Columns
    {
        IdCol,
        NameCol = IdCol,
        FreshwaterCol = IdCol,

        LvlCol
    };

    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        LvlRole = Qt::UserRole + 1,
        FreshwaterRole
    };

    FishDbTreeModel(QObject* parent = nullptr);

    void buildTree();

    QModelIndex addRegion(QString name = "New Region");
    QModelIndex addZone(const QModelIndex& regionIdx, QString zone = "New Zone");
    QModelIndex addSpot(const QModelIndex& zoneIdx, QString name = "Undiscovered", int level = 1, bool freshwater = true);

    void moveUp(const QModelIndex& idx);
    void moveDown(const QModelIndex& idx);

    void setFreshwater(const QModelIndex& idx, bool isFreshwater);

    QStandardItem* getSpotItem(QString spotId);

signals:
    void spotChanged();

public slots:
    void handleDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

protected:
    QString getTableFromItem(const QStandardItem* item) const;

    bool exec(QString queryStr);

private:
    QHash<QString, QStandardItem*> m_spotItemMap; // Maps spot IDs to the item for each spot id
};
