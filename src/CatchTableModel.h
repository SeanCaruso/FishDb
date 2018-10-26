
#pragma once

#include <QStandardItemModel>

class CatchTableModel : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Roles
    {
        IdRole = Qt::UserRole + 1,
        NameRole,
        LevelRole,
        CatchRole
    };

    CatchTableModel(QObject* parent = nullptr);

    void clear();

    void setSpot(QString spotId, bool forceReset = false);

    void setBaitLevel(const QModelIndex& index, int level);
    void setFishLevel(const QModelIndex& index, int level);

    bool addOrSubtractCatch(const QModelIndex& index, bool add);

public slots:
    void handleBaitNameChanged(const QModelIndex& index);
    void handleFishNameChanged(const QModelIndex& index);

protected:
    void updateCatches(int row);
    void appendBaitRow();

private:
    QString m_currentSpotId;
    int m_freshwater;
    int m_numFish;
};
