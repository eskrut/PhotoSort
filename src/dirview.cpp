#include "dirview.h"

DirView::DirView(QWidget *parent) :
    QListView(parent)
{

    setDragEnabled(true);
    setDropIndicatorShown(true);
}
