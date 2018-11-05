
#pragma once

#include <QMap>
#include <QWidget>

class TriadWidget : public QWidget
{
    Q_OBJECT

    enum Result
    {
        Lose,
        Win,
        Draw
    };

    struct Move
    {
        QString card;
        quint8 pos;

        Move* previousMove;
        QList<Move*> nextMoves;
        Result result; // If nextMove is null, this indicates whether or not this move resulted in a win.

        Move() : previousMove(nullptr) {}
    };

    struct Game
    {

    };

public:
    explicit TriadWidget(QWidget *parent = nullptr);
    ~TriadWidget();

protected:
    void initializeFromFile();

private:
    QMap<QString, Move> m_games;
};
