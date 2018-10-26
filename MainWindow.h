
#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;
class QTableView;
class QTreeView;

class CatchTableModel;
class CatchTableView;
class FishDbTreeModel;

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget* parent = nullptr);

protected slots:
    void handleNew();

    void handleAddRegion();
    void handleAddZone();
    void handleAddSpot();

    void handleMoveUp();
    void handleMoveDown();

    void handleTreeColumnResize();
    void handleTreeDataChanged();
    void handleTreeSelectionChanged();
    void handleTreeRightClick(const QPoint& pos);

    void handleTableDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void handleTableRightClick(const QPoint& pos);

    void handleRemoveFishCol();
    void handleAddFishCol();

protected:
    void setSpot(QString spotId, bool forceReset);
    void setNumberOfFish(QString spotId, int numFish);

    void closeEvent(QCloseEvent* event);

private:
    QPushButton* m_addRegion;
    QPushButton* m_addZone;
    QPushButton* m_addSpot;
    QPushButton* m_moveUp;
    QPushButton* m_moveDown;

    QTreeView* m_tree;
    FishDbTreeModel* m_treeModel;

    QLabel* m_tableLabel;
    CatchTableView* m_table;
    CatchTableModel* m_tableModel;

    QPushButton* m_removeFishCol;
    QPushButton* m_addFishCol;
};
