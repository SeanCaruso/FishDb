
#include <QComboBox>
#include <QCompleter>
#include <QHash>
#include <QLayout>
#include <QListWidget>
#include <QSqlQuery>

#include "FishDb.h"

#include "FishBrowser.h"

FishBrowser::FishBrowser(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QComboBox* fishCombo = new QComboBox(this);
    fishCombo->setEditable(true);
    layout->addWidget(fishCombo);

    QStringList fishNames;
    QSqlQuery query(FishDb::db());
    query.exec("SELECT id, name, level FROM fish ORDER BY name");
    while (query.next())
    {
        QString text = query.value(1).toString(); // +" (Lv. " + query.value(2).toString() + ")";
        fishCombo->addItem(text, query.value(0).toString());
    }

    connect(fishCombo, (void(QComboBox::*)(int)) &QComboBox::currentIndexChanged, [=](int index)
    {
        QString id = fishCombo->itemData(index).toString();
        lookupFish(id);
    });

    m_resultList = new QListWidget(this);
    layout->addWidget(m_resultList);
}

FishBrowser::~FishBrowser()
{
}

void FishBrowser::lookupFish(QString id)
{
    m_resultList->clear();

    QList<Catch> catches;

    QSqlQuery spotQuery(FishDb::db());
    spotQuery.exec("SELECT zones.name, spots.name, spots.id FROM zones, spots, spot_fish "
        "WHERE spot_fish.fish_id = " + id + " AND "
        "spot_fish.spot_id = spots.id AND "
        "spots.zone_id = zones.id");
    while (spotQuery.next())
    {
        QString zoneName = spotQuery.value(0).toString();
        QString spotName = spotQuery.value(1).toString();
        QString spotId = spotQuery.value(2).toString();

        // Hashes of bait strings to catches of this fish and total catches.
        QHash<QString, int> fishCatches;
        QHash<QString, int> totalCatches;

        // Build the hashes.
        QSqlQuery catchQuery(FishDb::db());
        catchQuery.exec("SELECT bait.name, bait.level, fish_id, count"
            " FROM catches, bait"
            " WHERE catches.spot_id = " + spotId +
            " AND catches.bait_id = bait.id");
        while (catchQuery.next())
        {
            QString baitName = catchQuery.value(0).toString();
            QString baitLvl = catchQuery.value(1).toString();
            QString fishId = catchQuery.value(2).toString();
            int count = catchQuery.value(3).toInt();

            QString baitText = baitName; // +" (Lv. " + baitLvl + ")";

            if (fishId == id)
                fishCatches[baitText] = count;

            if (totalCatches.contains(baitText))
                totalCatches[baitText] += count;
            else
                totalCatches[baitText] = count;
        }

        // Now create the Catch objects and insert them in the correct order.
        for (auto itr = fishCatches.begin(); itr != fishCatches.end(); ++itr)
        {
            if (itr.value() == 0)
                continue;

            Catch newCatch;
            QString baitText = itr.key();
            newCatch.text = spotName + " (" + baitText + "), " + zoneName;
            newCatch.catches = itr.value();
            newCatch.total = totalCatches[baitText];
            newCatch.percent = ((float)itr.value() / (float)totalCatches[baitText]) * 100;

            insertCatch(newCatch, catches);
        }
    }

    // Now that we have all Catch objects for all spots, build the list widget.
    for (auto itr = catches.begin(); itr < catches.end(); ++itr)
    {
        QString text = QString::number(itr->percent) + "% (" + QString::number(itr->catches) +
            "/" + QString::number(itr->total) + ") - " + itr->text;
        m_resultList->addItem(text);
    }
    
    m_resultList->setMinimumWidth(m_resultList->sizeHintForColumn(0) + 5);
}

void FishBrowser::insertCatch(const Catch& newCatch, QList<Catch>& catches) const
{
    int index = 0;

    // Insert higher percentages first.
    while (catches.size() > index &&
           catches[index].percent > newCatch.percent)
    {
        ++index;
    }

    // If the percentage is equal, insert higher total catches first.
    while (catches.size() > index &&
           catches[index].percent == newCatch.percent &&
           catches[index].catches > newCatch.catches)
    {
        ++index;
    }

    // If the percentage and total catches are equal, just sort by text.
    while (catches.size() > index &&
           catches[index].percent == newCatch.percent &&
           catches[index].catches == newCatch.catches &&
           catches[index].text < newCatch.text)
    {
        ++index;
    }

    catches.insert(index, newCatch);
}