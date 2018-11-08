
#pragma once

#include <QStandardItemModel>

class CatchTableModel : public QStandardItemModel
{
    Q_OBJECT

public:

    static const int g_invalidLevel;

    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        NameRole,  // Bait/fish name
        LevelRole, // Bait/fish level
        CatchRole, // Catch count
        FreshwaterRole
    };

    CatchTableModel(QObject* parent = nullptr);

    void clear();

    void setSpot(QString spotId, bool forceReset = false);

    void setBaitLevel(const QModelIndex& index, int level);
    void setFishLevel(const QModelIndex& index, int level);

    bool addOrSubtractCatch(const QModelIndex& index, bool add);

public slots:
    void handleBaitChanged(const QModelIndex& index);
    void handleFishChanged(const QModelIndex& index);

protected:
    // This uses the item data in the row to update items' text with percentages.
    void updateCatches(int row);
    void appendBaitRow();

private:
    QString m_currentSpotId;
    int m_freshwater;
    int m_numFish;
};
