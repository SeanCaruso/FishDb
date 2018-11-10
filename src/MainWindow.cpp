
#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSqlQuery>
#include <QTableView>
#include <QTreeView>

#include "CatchTableDelegate.h"
#include "CatchTableModel.h"
#include "CatchTableView.h"
#include "FishBrowser.h"
#include "FishDb.h"
#include "FishDbTreeDelegate.h"
#include "FishDbTreeModel.h"

#include "MainWindow.h"

// ISSUES TO LOOK AT:
// - (Low Priority) - Search/Filter for spots
// - (Low Priority) - Fish browser dialog that allows removing of unused (i.e. mistake) fish

// FISH LOGS (10-28 db up to date):

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setStyleSheet(
        "font-family: Calibri; "
        "font-size: 14px; "
    );

    FishDb::open("D:/src/fishdb.db");

    QWidget* mainWidget = new QWidget(this);
    setCentralWidget(mainWidget);

    QGridLayout* mainLayout = new QGridLayout(mainWidget);

    QVBoxLayout* buttonLayout = new QVBoxLayout;

    m_addRegion = new QPushButton("Add Region...", this);
    connect(m_addRegion, &QPushButton::clicked, this, &MainWindow::handleAddRegion);
    buttonLayout->addWidget(m_addRegion);

    m_addZone = new QPushButton("Add Zone...", this);
    m_addZone->setEnabled(false);
    connect(m_addZone, &QPushButton::clicked, this, &MainWindow::handleAddZone);
    buttonLayout->addWidget(m_addZone);

    m_addSpot = new QPushButton("Add Spot...", this);
    m_addSpot->setEnabled(false);
    connect(m_addSpot, &QPushButton::clicked, this, &MainWindow::handleAddSpot);
    buttonLayout->addWidget(m_addSpot);

    QHBoxLayout* upDownLayout = new QHBoxLayout;
    m_moveUp = new QPushButton(style()->standardIcon(QStyle::SP_ArrowUp), "");
    m_moveUp->setEnabled(false);
    connect(m_moveUp, &QPushButton::clicked, this, &MainWindow::handleMoveUp);
    upDownLayout->addWidget(m_moveUp);

    m_moveDown = new QPushButton(style()->standardIcon(QStyle::SP_ArrowDown), "");
    m_moveDown->setEnabled(false);
    connect(m_moveDown, &QPushButton::clicked, this, &MainWindow::handleMoveDown);
    upDownLayout->addWidget(m_moveDown);
    buttonLayout->addLayout(upDownLayout);

    mainLayout->addLayout(buttonLayout, 1, 0, Qt::AlignVCenter);

    // Build the tree view that contains all the fishing spots. ======================================================
    QVBoxLayout* treeLayout = new QVBoxLayout;
    mainLayout->addLayout(treeLayout, 1, 1);

    m_tree = new QTreeView(this);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    treeLayout->addWidget(m_tree);

    // Set up the tree header
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tree->header()->setVisible(false);
    connect(m_tree->header(), &QHeaderView::sectionResized, this, &MainWindow::handleTreeColumnResize);

    m_treeModel = new FishDbTreeModel(this);
    m_tree->setModel(m_treeModel);
    m_tree->setItemDelegate(new FishDbTreeDelegate(this));
    m_tree->expandAll();
    handleTreeColumnResize(); // Manually call this here to initialize to the correct size.
    m_tree->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::handleTreeSelectionChanged);
    connect(m_treeModel, &FishDbTreeModel::spotChanged, this, &MainWindow::handleTreeSelectionChanged);

    // Custom context menu handling
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &MainWindow::handleTreeRightClick);

    // Tree filtering
    QLineEdit* treeFilter = new QLineEdit(this);
    treeFilter->setPlaceholderText("Filter spots...");
    treeFilter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    treeLayout->addWidget(treeFilter);

    connect(treeFilter, &QLineEdit::textChanged, this, &MainWindow::handleTreeFilter);

    // Build the table view that contains all the catch info. ==========================================================
    m_tableLabel = new QLabel("", this);
    QFont font = m_tableLabel->font();
    font.setPointSize(14);
    m_tableLabel->setFont(font);
    mainLayout->addWidget(m_tableLabel, 0, 2);

    m_table = new CatchTableView(this);

    m_tableModel = new CatchTableModel(this);
    m_table->setModel(m_tableModel);
    CatchTableDelegate* tableDelegate = new CatchTableDelegate(this);
    m_table->setItemDelegate(tableDelegate);
    mainLayout->addWidget(m_table, 1, 2);

    connect(m_table->model(), &QAbstractItemModel::dataChanged, this, &MainWindow::handleTableDataChanged);

    connect(tableDelegate, &CatchTableDelegate::baitChanged, m_tableModel, &CatchTableModel::handleBaitChanged);
    connect(tableDelegate, &CatchTableDelegate::fishChanged, m_tableModel, &CatchTableModel::handleFishChanged);

    // Custom context menu handling
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTreeView::customContextMenuRequested, this, &MainWindow::handleTableRightClick);

    // Collapse/Expand Buttons =========================================================================================
    QHBoxLayout* collapseLayout = new QHBoxLayout;
    mainLayout->addLayout(collapseLayout, 2, 1, Qt::AlignCenter);

    QPushButton* expandButton = new QPushButton("Expand All");
    collapseLayout->addWidget(expandButton);
    connect(expandButton, &QPushButton::clicked, [=]()
    {
        this->m_tree->expandAll();
    });

    QPushButton* collapseButton = new QPushButton("Collapse All");
    collapseLayout->addWidget(collapseButton);
    connect(collapseButton, &QPushButton::clicked, [=]()
    {
        this->m_tree->collapseAll();
    });

    // Add/Remove Fish buttons =========================================================================================
    QHBoxLayout* subTableLayout = new QHBoxLayout;
    mainLayout->addLayout(subTableLayout, 2, 2);

    m_removeFishCol = new QPushButton("-");
    m_removeFishCol->setEnabled(false);
    connect(m_removeFishCol, &QPushButton::clicked, this, &MainWindow::handleRemoveFishCol);
    subTableLayout->addWidget(m_removeFishCol);

    m_addFishCol = new QPushButton("+");
    m_addFishCol->setEnabled(false);
    connect(m_addFishCol, &QPushButton::clicked, this, &MainWindow::handleAddFishCol);
    subTableLayout->addWidget(m_addFishCol);

    subTableLayout->addStretch();

    // Always-on-top ===================================================================================================
    QCheckBox* onTop = new QCheckBox("Always On Top?");
    onTop->setCheckState(Qt::Unchecked);
    subTableLayout->addWidget(onTop);
    connect(onTop, &QCheckBox::toggled, [=](bool checked)
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        this->show();
    });

    // Now that everything is set up, add and connect menu items. ======================================================
    // File Menu -------------------------------------
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAction = new QAction("&New", this);
    fileMenu->addAction(newAction);
    connect(newAction, &QAction::triggered, this, &MainWindow::handleNew);

    QAction* exportAction = new QAction("&Export...", this);
    fileMenu->addAction(exportAction);
    connect(exportAction, &QAction::triggered, this, &MainWindow::handleExport);

    fileMenu->addSeparator();

    QAction* quitAction = new QAction("&Quit", this);
    fileMenu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    // Tools Menu ------------------------------------
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* fishAction = new QAction("&Fish Browser", this);
    fishAction->setShortcut(QKeySequence::Find);
    toolsMenu->addAction(fishAction);
    connect(fishAction, &QAction::triggered, [=]()
    {
        FishBrowser* dlg = new FishBrowser(this);

        connect(dlg, &FishBrowser::spotDoubleClicked, this, &MainWindow::setTreeSpot);

        dlg->exec();
    });

    // Restore last window position.
    QSettings settings;
    settings.beginGroup("mainwindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    settings.endGroup();
}

void MainWindow::handleExport()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Database", "D:/src/fishdb.xml", "XML files (*.xml)");
    if (!fileName.isEmpty())
        FishDb::exportDb(fileName);
}

void MainWindow::handleNew()
{
    QMessageBox::StandardButton result = QMessageBox::warning(
                this,
                "NEW DATABASE WARNING",
                "This will completely destroy the database. Are you sure?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
    );

    if (result == QMessageBox::Yes)
    {
        QString timeStr = QDateTime::currentDateTime().toString("MM-dd-yyyy_HHmmss");
        QFile::copy("D:/src/fishdb.db", "D:/src/fishdb_" + timeStr + ".db");

        FishDb::createNew();
        m_treeModel->clear();
        m_tableLabel->clear();
        m_tableModel->clear();

        handleTreeSelectionChanged();
    }
}

void MainWindow::handleAddRegion()
{
    QModelIndex index = m_treeModel->addRegion();
    m_tree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    m_tree->edit(index);
}

void MainWindow::handleAddZone()
{
    // We want the index of the region of the selected item.
    QModelIndex regionIdx = m_tree->currentIndex();
    while (regionIdx.parent().isValid())
    {
        regionIdx = regionIdx.parent();
    }

    QModelIndex zoneIdx = m_treeModel->addZone(regionIdx);
    m_tree->expand(regionIdx);
    m_tree->selectionModel()->setCurrentIndex(zoneIdx, QItemSelectionModel::ClearAndSelect);
    m_tree->edit(zoneIdx);
}

void MainWindow::handleAddSpot()
{
    // If the selected item has two parents, we're currently on a spot and want the current item's parent.
    QModelIndex zoneIdx = m_tree->currentIndex();
    if (zoneIdx.parent().parent().isValid())
    {
        zoneIdx = zoneIdx.parent();
    }

    QModelIndex spotIdx = m_treeModel->addSpot(zoneIdx);
    QString test = spotIdx.data(FishDbTreeModel::IdRole).toString();
    m_tree->expand(zoneIdx);
    m_tree->selectionModel()->setCurrentIndex(spotIdx, QItemSelectionModel::ClearAndSelect);
    m_tree->edit(spotIdx);
}

void MainWindow::handleMoveUp()
{
    QModelIndex curIdx = m_tree->currentIndex();
    bool expanded = m_tree->isExpanded(curIdx);
    m_treeModel->moveUp(curIdx);

    m_tree->setCurrentIndex(curIdx.sibling(curIdx.row() - 1, curIdx.column()));
    m_tree->setExpanded(m_tree->currentIndex(), expanded);
}

void MainWindow::handleMoveDown()
{
    QModelIndex curIdx = m_tree->currentIndex();
    bool expanded = m_tree->isExpanded(curIdx);
    m_treeModel->moveDown(curIdx);

    m_tree->setCurrentIndex(curIdx.sibling(curIdx.row() + 1, curIdx.column()));
    m_tree->setExpanded(m_tree->currentIndex(), expanded);
}

void MainWindow::handleTreeColumnResize()
{
    int width = m_tree->header()->length();
    width += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    m_tree->setMinimumWidth(width + 8);
}

void MainWindow::handleTreeSelectionChanged()
{
    bool addZone = false;
    bool addSpot = false;
    bool moveUp = false;
    bool moveDown = false;

    QString labelText = "";

    QModelIndex currentIdx = m_tree->currentIndex();
    // If nothing is selected, disable everything but Add Region.
    if (!currentIdx.isValid())
    {
        m_tableModel->clear();
    }
    // If there's no parent, a region is selected.
    else if (!currentIdx.parent().isValid())
    {
        addZone = true;
        m_tableModel->clear();
    }
    // If there's one parent, a zone is selected.
    else if (!currentIdx.parent().parent().isValid())
    {
        addZone = true;
        addSpot = true;
        m_tableModel->clear();
    }
    // If there's two parents, a spot is selected.
    else
    {
        QString spotName = currentIdx.sibling(currentIdx.row(), FishDbTreeModel::NameCol).data(Qt::DisplayRole).toString();
        QString spotLevel = currentIdx.sibling(currentIdx.row(), FishDbTreeModel::LvlCol).data(FishDbTreeModel::LvlRole).toString();
        QString freshwater = currentIdx.sibling(currentIdx.row(), FishDbTreeModel::FreshwaterCol).data(FishDbTreeModel::FreshwaterRole).toBool() ? "Freshwater" : "Saltwater";
        labelText = spotName + " (Lv. " + spotLevel + " " + freshwater + ")";

        QString spotId = currentIdx.sibling(currentIdx.row(), FishDbTreeModel::IdCol).data(FishDbTreeModel::IdRole).toString();
        setTableSpot(spotId, false);

        addZone = true;
        addSpot = true;
    }

    moveUp = currentIdx.row() > 0;
    moveDown = currentIdx.sibling(currentIdx.row() + 1, currentIdx.column()).isValid();

    m_addZone->setEnabled(addZone);
    m_addSpot->setEnabled(addSpot);
    m_moveUp->setEnabled(moveUp);
    m_moveDown->setEnabled(moveDown);

    m_tableLabel->setText(labelText);
}

void MainWindow::handleTreeRightClick(const QPoint& pos)
{
    QModelIndex clickedIndex = m_tree->indexAt(pos);
    QModelIndex index = clickedIndex.sibling(clickedIndex.row(), 0);

    // Only show context menus for spots - these have two parents.
    if (!index.parent().parent().isValid())
        return;

    QString spotId = index.data(FishDbTreeModel::IdRole).toString();
    bool isFreshwater = index.data(FishDbTreeModel::FreshwaterRole).toBool();

    QMenu menu(this);

    QAction* numFishAction = new QAction("Set &Number of Fish...");
    menu.addAction(numFishAction);

    menu.addSeparator();

    QAction* freshAction = new QAction("&Freshwater");
    freshAction->setCheckable(true);
    freshAction->setChecked(isFreshwater);
    menu.addAction(freshAction);

    QAction* saltAction = new QAction("&Saltwater");
    saltAction->setCheckable(true);
    saltAction->setChecked(!isFreshwater);
    menu.addAction(saltAction);
    
    menu.addSeparator();

    QMenu* moveToMenu = new QMenu("&Move To");
    menu.addMenu(moveToMenu);

    QSqlQuery query(FishDb::db());
    query.exec("SELECT id, name FROM zones ORDER BY name");
    while (query.next())
    {
        QAction* action = new QAction(query.value(1).toString());
        action->setData(query.value(0));
        moveToMenu->addAction(action);
    }

    QAction* deleteAction = new QAction("&Delete");
    menu.addAction(deleteAction);

    QAction* selected = menu.exec(m_tree->mapToGlobal(pos));
    if (!selected)
        return;

    if (selected == numFishAction)
    {
        int curNumFish = m_tableModel->columnCount() - 1;

        bool ok;
        int numFish = QInputDialog::getInt(this, "Number of Fish", "Number of Fish", curNumFish, 1, 15, 1, &ok);
        if (ok && numFish != curNumFish)
        {
            setNumberOfFish(spotId, numFish);
        }
    }
    // Update the freshwater status only if it's different from the current setting.
    else if ((selected == freshAction && !isFreshwater) || (selected == saltAction && isFreshwater))
    {
        m_treeModel->setFreshwater(index, selected == freshAction);
        handleTreeSelectionChanged();
        setTableSpot(index.data(FishDbTreeModel::IdRole).toString(), true);
    }
    else if (selected->associatedWidgets().at(0) == moveToMenu)
    {
        query.exec("SELECT sort_order FROM spots WHERE id = " + spotId);
        query.next();
        int sortOrder = query.value(0).toInt() + 1;

        query.exec("UPDATE spots SET (zone_id, sort_order) = (" +
            selected->data().toString() + ", " + QString::number(sortOrder) + ") WHERE id = " + spotId);
        // Redraw the tree :-/
        m_treeModel->buildTree();
        handleTreeSelectionChanged();
    }
    else if (selected == deleteAction)
    {
        QMessageBox::StandardButton result = QMessageBox::warning(
            this,
            "Are you sure?",
            "Deleting this item is permanent. Are you sure?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (result == QMessageBox::Yes)
        {
            query.exec("DELETE FROM spots WHERE id = " + spotId);
            // These should be handled by the above call, but just in case...
            query.exec("DELETE FROM spot_fish WHERE spot_id = " + spotId);
            query.exec("DELETE FROM catches WHERE spot_id = " + spotId);
            // Redraw the tree :-/
            m_treeModel->itemFromIndex(index.parent())->removeRow(index.row());
            handleTreeSelectionChanged();
        }
    }
}

void MainWindow::handleTreeFilter(QString text)
{
    QString searchText = text.toUpper();

    QModelIndex singleSpotIdx;
    int resultCount = 0;

    for (int regionRow = 0; regionRow < m_treeModel->rowCount(); ++regionRow)
    {
        bool hideRegion = !text.isEmpty(); // If the search text is empty, default to visible.
        QModelIndex regionIdx = m_treeModel->index(regionRow, 0);
        QStandardItem* regionItem = m_treeModel->itemFromIndex(regionIdx);

        for (int zoneRow = 0; zoneRow < regionItem->rowCount(); ++zoneRow)
        {
            bool hideZone = !text.isEmpty(); // If the search text is empty, default to visible.
            QModelIndex zoneIdx = m_treeModel->index(zoneRow, 0, regionIdx);
            QStandardItem* zoneItem = m_treeModel->itemFromIndex(zoneIdx);

            for (int spotRow = 0; spotRow < zoneItem->rowCount(); ++spotRow)
            {
                QModelIndex spotIdx = m_treeModel->index(spotRow, 0, zoneIdx);
                QString spotName = spotIdx.data().toString().toUpper();
                // If we can't find the search string in the spot name, and we've entered
                // something to filter by, hide this row.
                if (!spotName.contains(searchText) && !text.isEmpty())
                {
                    m_tree->setRowHidden(spotRow, zoneIdx, true);

                    // If this is the currently selected item, clear the table.
                    if (m_tree->currentIndex() == spotIdx)
                    {
                        m_tree->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::ClearAndSelect);
                    }
                }
                // Otherwise, show the row and indicate that we should also show the zone and region.
                else
                {
                    m_tree->setRowHidden(spotRow, zoneIdx, false);
                    hideZone = false;
                    hideRegion = false;

                    // Increment the number of results, and keep this spot's index around for later.
                    singleSpotIdx = spotIdx;
                    ++resultCount;
                }
            }

            m_tree->setRowHidden(zoneRow, regionIdx, hideZone);
        }

        m_tree->setRowHidden(regionRow, QModelIndex(), hideRegion);
    }

    // If we're left with a single spot, set that as the selected item.
    if (resultCount == 1)
    {
        m_tree->setCurrentIndex(singleSpotIdx);
    }
}

void MainWindow::handleTableDataChanged(const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&)
{
    m_table->resizeColumn(topLeft.column());
}

void MainWindow::handleTableRightClick(const QPoint& pos)
{
    QModelIndex index = m_table->indexAt(pos);

    // If the item has an ID, we're modifying a known bait or fish.
    if (index.data(CatchTableModel::IdRole).isValid())
    {
        QMenu menu(this);
        QAction* setLevelAction = new QAction("Set &Level...");
        menu.addAction(setLevelAction);

        QAction* deleteAction = nullptr;
        if (index.column() == 0)
        {
            menu.addSeparator();

            deleteAction = new QAction("&Delete Bait");
            menu.addAction(deleteAction);
        }

        QAction* action = menu.exec(m_table->mapToGlobal(pos));
        if (action == setLevelAction)
        {
            int curLvl = index.data(CatchTableModel::LevelRole).toInt();
            bool ok;
            QString message = index.column() == 0 ? "Bait Level" : "Fish Level";
            int lvl = QInputDialog::getInt(this, "Enter Level", message, curLvl, 1, 70, 1, &ok);
            if (ok)
            {
                if (index.column() == 0)
                {
                    m_tableModel->setBaitLevel(index, lvl);
                }
                else
                {
                    m_tableModel->setFishLevel(index, lvl);
                }
            }
        }
        else if (action && action == deleteAction)
        {
            QMessageBox::StandardButton result = QMessageBox::warning(
                this,
                "Are you sure?",
                "Deleting this item is permanent. Are you sure?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );

            if (result == QMessageBox::Yes)
            {
                QString id = index.data(CatchTableModel::IdRole).toString();
                QSqlQuery query(FishDb::db());
                query.exec("DELETE FROM catches WHERE bait_id = " + id);
                query.exec("DELETE FROM bait WHERE id = " + id);

                m_tableModel->removeRow(index.row());
            }
        }
    }
}

void MainWindow::handleRemoveFishCol()
{
    QModelIndex treeIdx = m_tree->currentIndex();
    QModelIndex spotIdx = treeIdx.sibling(treeIdx.row(), FishDbTreeModel::IdCol);
    QString spotId = spotIdx.data(FishDbTreeModel::IdRole).toString();
    setNumberOfFish(spotId, m_tableModel->columnCount() - 2);
}

void MainWindow::handleAddFishCol()
{
    QModelIndex treeIdx = m_tree->currentIndex();
    QModelIndex spotIdx = treeIdx.sibling(treeIdx.row(), FishDbTreeModel::IdCol);
    QString spotId = spotIdx.data(FishDbTreeModel::IdRole).toString();
    setNumberOfFish(spotId, m_tableModel->columnCount());
}

void MainWindow::setTreeSpot(QString spotId)
{
    QStandardItem* item = m_treeModel->getSpotItem(spotId);
    if (item)
    {
        m_tree->setCurrentIndex(item->index());
        m_tree->expand(item->parent()->index());
        m_tree->expand(item->parent()->parent()->index());
    }
}

void MainWindow::setTableSpot(QString spotId, bool forceReset)
{
    m_tableModel->setSpot(spotId, forceReset);
    m_table->resizeColumns();

    int numCols = m_tableModel->columnCount();
    // Enable the button to remove a column if the last column has an unknown fish.
    QModelIndex lastFishIdx = m_tableModel->index(0, numCols - 1);
    m_removeFishCol->setEnabled(!lastFishIdx.data(CatchTableModel::IdRole).isValid());

    // Enable the button to add a column if we have less than 15 fish.
    m_addFishCol->setEnabled(numCols < 16);
}

void MainWindow::setNumberOfFish(QString spotId, int numFish)
{
    QSqlQuery query(FishDb::db());
    query.exec("UPDATE spots SET num_fish = " + QString::number(numFish) + " WHERE id = " + spotId);
    setTableSpot(spotId, true);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.beginGroup("mainwindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();
    QMainWindow::closeEvent(event);
}
