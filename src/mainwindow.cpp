#include "mainwindow.h"

#include "QBoxLayout"
#include "QHBoxLayout"
#include "QVBoxLayout"

#include <QDir>
#include <QDebug>
#include <QShortcut>
#include <QFileDialog>
#include <QThread>

#include "singleimageitem.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QFrame *frame = new QFrame;
    auto lo = new QHBoxLayout(frame);

    dirView_ = new QListView();
    lo->addWidget(dirView_);
    dirView_->setViewMode(QListView::IconMode);
    dirView_->setGridSize(QSize(300, 300));
    dirView_->setResizeMode(QListView::Adjust);

    setCentralWidget(frame);

    model_ = new DirModel;
    auto thread = new QThread;
    thread->start();
    model_->moveToThread(thread);
    dirView_->setModel(model_);
    dirView_->setSelectionModel(new QItemSelectionModel(model_));

    viewer_ = new Viewer(this);
    lo->addWidget(viewer_);
    viewer_->setMinimumWidth(200);

    setupSignals();
}

void MainWindow::onOpenDir(const QString &dirName)
{
    QDir dir(dirName);
    auto entries = dir.entryList(QStringList("*.JPG"));
    for(auto &e : entries){
        e = dirName + "/" + e;
    }
//    model_->setup(entries);
    emit read(entries);
}

void MainWindow::setupSignals()
{
    QShortcut *openDirShC = new QShortcut(QKeySequence(QKeySequence::Open), this);
    Q_ASSERT(connect(openDirShC, &QShortcut::activated, this, &MainWindow::onOpenRequest));

    Q_ASSERT(connect(model_, &DirModel::updateRequest, [=](){dirView_->update();}));
    Q_ASSERT(connect(this, &MainWindow::read, model_, &DirModel::setup));

    Q_ASSERT(connect(dirView_->selectionModel(), &QItemSelectionModel::selectionChanged, [&](QItemSelection sel,QItemSelection desel){
                 auto indexes = sel.indexes();
                 if (indexes.size() == 1) {
                     viewer_->viewItem(model_->itemFromIndex(indexes.front()));
//                     viewer_->view(indexes.front().data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString());
//                     update();
                 }
             }));
}

void MainWindow::onOpenRequest()
{
    QString dirName = QFileDialog::getExistingDirectory(this);
    if(!dirName.isEmpty()) onOpenDir(dirName);
}
