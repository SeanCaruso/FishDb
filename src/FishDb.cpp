
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QXmlStreamWriter>

#include "FishDb.h"

static const int g_curVersion = 3;

QSqlDatabase FishDb::db()
{
    return QSqlDatabase::database("FishDb");
}

void FishDb::createNew()
{
    QSqlDatabase theDb = QSqlDatabase::addDatabase("QSQLITE", "FishDb");
    theDb.setDatabaseName("D:/src/fishdb.db");
    theDb.open();

    QSqlQuery query(theDb);
    // Drop tables
    query.exec("DROP TABLE IF EXISTS catches");
    query.exec("DROP TABLE IF EXISTS spot_fish");
    query.exec("DROP TABLE IF EXISTS fish");
    query.exec("DROP TABLE IF EXISTS bait");
    query.exec("DROP TABLE IF EXISTS spots");
    query.exec("DROP TABLE IF EXISTS zones");
    query.exec("DROP TABLE IF EXISTS regions");

    // Create tables
    // Regions
    query.exec("CREATE TABLE IF NOT EXISTS regions("
        "id INTEGER PRIMARY KEY, name, sort_order INTEGER)");
    // Zones
    query.exec("CREATE TABLE IF NOT EXISTS zones("
        "id INTEGER PRIMARY KEY, region_id REFERENCES regions(id) ON DELETE CASCADE, name, sort_order INTEGER)");
    // Spots
    query.exec("CREATE TABLE IF NOT EXISTS spots("
        "id INTEGER PRIMARY KEY, zone_id REFERENCES zones(id) ON DELETE CASCADE, name, level INTEGER, freshwater, sort_order INTEGER, num_fish)");
    // Bait
    query.exec("CREATE TABLE IF NOT EXISTS bait("
        "id INTEGER PRIMARY KEY, name UNIQUE, level INTEGER, freshwater)");
    // Fish
    query.exec("CREATE TABLE IF NOT EXISTS fish("
        "id INTEGER PRIMARY KEY, name UNIQUE, level INTEGER)");
    // Spot_Fish
    query.exec("CREATE TABLE IF NOT EXISTS spot_fish("
        "spot_id REFERENCES spots(id), fish_id REFERENCES fish(id) ON DELETE CASCADE, sort_order INTEGER)");
    // Catches
    query.exec("CREATE TABLE IF NOT EXISTS catches("
        "spot_id REFERENCES spots(id), bait_id REFERENCES bait(id) ON DELETE CASCADE, fish_id REFERENCES fish(id), count)");

    // Create indices
    query.exec("CREATE UNIQUE INDEX idx_spot_fish_fish ON spot_fish (spot_id, fish_id)");
    query.exec("CREATE UNIQUE INDEX idx_spot_fish_sort ON spot_fish (spot_id, sort_order)");
    query.exec("CREATE UNIQUE INDEX idx_catch ON catches (spot_id, bait_id, fish_id)");

    // Set the current version.
    query.exec("PRAGMA user_version = " + QString::number(g_curVersion));
}

void FishDb::open(QString path)
{
    db().close();

    QSqlDatabase theDb = QSqlDatabase::addDatabase("QSQLITE", "FishDb");
    theDb.setDatabaseName(path);
    theDb.open();

    // Make any database changes here. ============================================
    // Perform clean-up.
    QSqlQuery cleanupQuery(theDb);
    cleanupQuery.exec("DELETE FROM spot_fish WHERE fish_id NOT IN (SELECT id FROM fish)");
    cleanupQuery.exec("DELETE FROM catches WHERE fish_id NOT IN (SELECT id FROM fish)");

    QSqlQuery versionQuery(theDb);
    versionQuery.exec("PRAGMA user_version");
    versionQuery.next();
    int version = versionQuery.value(0).toInt();
    versionQuery.finish(); // Manually finish this to prevent row locks.

    // Version 2 added the num_fish column
    if (version < 2)
    {
        QSqlQuery query(theDb);
        query.exec("ALTER TABLE spots ADD COLUMN num_fish DEFAULT 9");
    }
    // Version 3 updated the indices on spot_fish.
    if (version < 3)
    {
        QSqlQuery query(theDb);
        query.exec("DROP INDEX IF EXISTS idx_spot_fish");
        query.exec("CREATE UNIQUE INDEX idx_spot_fish_fish ON spot_fish (spot_id, fish_id)");
        query.exec("CREATE UNIQUE INDEX idx_spot_fish_sort ON spot_fish (spot_id, sort_order)");
    }

    versionQuery.exec("PRAGMA user_version = " + QString::number(g_curVersion));
}

void FishDb::exportDb(QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("FishDb");

    QSqlQuery regionQuery(db());
    regionQuery.exec("SELECT id, name FROM regions ORDER BY sort_order");
    while (regionQuery.next())
    {
        stream.writeStartElement("region");
        stream.writeAttribute("name", regionQuery.value(1).toString());

        QSqlQuery zoneQuery(db());
        zoneQuery.exec("SELECT id, name FROM zones WHERE region_id = " + regionQuery.value(0).toString() + " ORDER BY sort_order");
        while (zoneQuery.next())
        {
            stream.writeStartElement("zone");
            stream.writeAttribute("name", zoneQuery.value(1).toString());

            QSqlQuery spotQuery(db());
            spotQuery.exec("SELECT id, name, level, freshwater, num_fish FROM spots WHERE zone_id = " + zoneQuery.value(0).toString() + " ORDER BY sort_order");
            while (spotQuery.next())
            {
                stream.writeStartElement("spot");
                stream.writeAttribute("name", spotQuery.value(1).toString());
                stream.writeAttribute("level", spotQuery.value(2).toString());
                stream.writeAttribute("freshwater", spotQuery.value(3).toBool() ? "true" : "false");
                stream.writeAttribute("num_fish", spotQuery.value(4).toString());

                QSqlQuery fishQuery(db());
                QString spotId = spotQuery.value(0).toString();
                fishQuery.exec("SELECT fish.id, name, level, sort_order FROM fish, spot_fish WHERE spot_id = " + spotId + " AND spot_fish.fish_id = fish.id ORDER BY sort_order");
                while (fishQuery.next())
                {
                    stream.writeStartElement("fish");
                    stream.writeAttribute("name", fishQuery.value(1).toString());
                    stream.writeAttribute("level", fishQuery.value(2).toString());
                    stream.writeAttribute("sort_order", fishQuery.value(3).toString());

                    QSqlQuery baitQuery(db());
                    baitQuery.exec("SELECT name, level, count FROM bait, catches WHERE spot_id = " + spotId + " AND catches.bait_id = bait.id AND fish_id = " + fishQuery.value(0).toString() + " ORDER BY level");
                    while (baitQuery.next())
                    {
                        stream.writeStartElement("bait");
                        stream.writeAttribute("name", baitQuery.value(0).toString());
                        stream.writeAttribute("level", baitQuery.value(1).toString());
                        stream.writeAttribute("count", baitQuery.value(2).toString());
                        stream.writeEndElement(); // bait
                    }
                    stream.writeEndElement(); // fish
                }
                stream.writeEndElement(); // spot
            }
            stream.writeEndElement(); // zone
        }
        stream.writeEndElement(); // region
    }
    stream.writeEndDocument(); // FishDb

    stream.writeEndDocument();
}
