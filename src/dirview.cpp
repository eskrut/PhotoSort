#include "dirview.h"

#include <qevent.h>

DirView::DirView(QWidget *parent) :
    QListView(parent)
{
    setSelectionMode(QListView::ExtendedSelection);
    setDragEnabled(true);
    setDropIndicatorShown(true);
}

void DirView::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Right) {
        auto indexes = selectionModel()->selectedIndexes();
        if(indexes.size() > 0) {
            auto index = indexes.front();
            if(index.row() < model()->rowCount() - 1) {
                selectionModel()->clear();
                selectionModel()->select(model()->index(index.row()+1, index.column(), index.parent()), QItemSelectionModel::Select);
            }
        }
        event->accept();
        return;
    }
    if(event->key() == Qt::Key_Left) {
        auto indexes = selectionModel()->selectedIndexes();
        if(indexes.size() > 0) {
            auto index = indexes.front();
            if(index.row() > 0) {
                selectionModel()->clear();
                selectionModel()->select(model()->index(index.row()-1, index.column(), index.parent()), QItemSelectionModel::Select);
            }
        }
        event->accept();
        return;
    }
    QListView::keyPressEvent(event);
}
