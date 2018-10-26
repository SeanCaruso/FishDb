
#pragma once

#include <QSqlDatabase>

namespace FishDb
{
    QSqlDatabase db();

    void createNew();
    void open(QString path);
};
