
#include <QFile>

#include "TriadWidget.h"

TriadWidget::TriadWidget(QWidget *parent) : QWidget(parent)
{


    initializeFromFile();
}

TriadWidget::~TriadWidget()
{
    QFile oFile("D:/src/fishdb_triad.dat");
    if (!oFile.open(QIODevice::ReadWrite))
        return;

    QDataStream out(&oFile);
    //out << m_games;

    oFile.close();
}

void TriadWidget::initializeFromFile()
{
    QFile file("D:/src/fishdb_triad.dat");
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&file);
    //in >> m_games;
}
