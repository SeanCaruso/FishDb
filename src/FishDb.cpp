
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

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
