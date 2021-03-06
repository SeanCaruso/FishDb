
#pragma once

#include <QDialog>

class QListWidget;

struct Catch
{
    int percent;
    int catches;
    int total;
    QString text;

    QString spotId;
};

class FishBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit FishBrowser(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~FishBrowser();

signals:
    void spotDoubleClicked(QString spotId) const;

protected:
    void lookupFish(QString id);

    void insertCatch(const Catch& newCatch, QList<Catch>& catches) const;

private:
    QListWidget* m_resultList;
};
