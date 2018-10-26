
#pragma once

#include <QStandardItemModel>

class CatchTableModel : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Roles
    {
        CatchRole = Qt::DisplayRole,

        IdRole = Qt::UserRole + 1,
        NameRole,
        LevelRole
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

protected slots:
    //void handleDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

protected:
    void appendBaitRow();

private:
    QString m_currentSpotId;
    int m_freshwater;
    int m_numFish;
};
