
#pragma once

#include <QSqlDatabase>
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

signals:
    void spotChanged();

public slots:
    void handleDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

protected:
    QString getTableFromItem(const QStandardItem* item) const;

    bool exec(QString queryStr);
};
