
#pragma once

#include <QTableView>

class CatchTableView : public QTableView
{
public:
    CatchTableView(QWidget* parent = nullptr);

    void resizeColumns();
    void resizeColumn(int col);

protected:
    void keyPressEvent(QKeyEvent* event);
    void wheelEvent(QWheelEvent* event);
};
