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
#include "groupedimages.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QFrame *frame = new QFrame;
    auto lo = new QHBoxLayout(frame);

    dirView_ = new DirView();
    lo->addWidget(dirView_);
    dirView_->setViewMode(QListView::IconMode);
    dirView_->setGridSize(QSize(300, 300));
    dirView_->setResizeMode(QListView::Adjust);
    dirView_->setMovement(QListView::Snap);

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

MainWindow::~MainWindow()
{
    delete model_;
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

    QShortcut *processShC = new QShortcut(QKeySequence(QKeySequence::Print), this);
    Q_ASSERT(connect(processShC, SIGNAL(activated()), model_, SLOT(process())));

    Q_ASSERT(connect(model_, &DirModel::updateRequest, [=](){dirView_->update();}));
    Q_ASSERT(connect(this, &MainWindow::read, model_, &DirModel::setup));

    Q_ASSERT(connect(dirView_->selectionModel(), &QItemSelectionModel::selectionChanged, [&](QItemSelection sel,QItemSelection desel){
                 auto indexes = sel.indexes();
                 if (indexes.size() == 1) {
                     viewer_->viewItem(model_->itemFromIndex(indexes.front()));
//                     viewer_->view(indexes.front().data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString());
//                     update();
                     viewer_->updateLabels();
                 }
             }));
    QShortcut *switchApprovShC = new QShortcut(QKeySequence("A"), this);
    Q_ASSERT(connect(switchApprovShC, &QShortcut::activated, [=](){
        auto indexes = dirView_->selectionModel()->selectedIndexes();
        for(auto i: indexes) {
            if(i.isValid()) {
                auto item = model_->itemFromIndex(i);
                if(dynamic_cast<SingleImageItem*>(item)) {
                    auto appr = item->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).value<bool>();
                    item->setData(QVariant::fromValue(!appr), static_cast<int>(SingleImageItem::Role::ApprovedRole));
                    model_->dataChanged(i, i);
                }
                if(dynamic_cast<GroupedImages*>(item)) {
                    auto gr = reinterpret_cast<GroupedImages*>(item);
                    auto single = gr->child(gr->curInFocus());
                    single->setData(!single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).value<bool>(), static_cast<int>(SingleImageItem::Role::ApprovedRole));
                    model_->dataChanged(single->index(), single->index());
                    model_->dataChanged(i, i);
                }
            }
            else
                qDebug() << QString("Item is not valid");
            viewer_->updateLabels();
        }
    }));
    QShortcut *switchApprovForceShC = new QShortcut(QKeySequence("F"), this);
    Q_ASSERT(connect(switchApprovForceShC, &QShortcut::activated, [=](){
        auto indexes = dirView_->selectionModel()->selectedIndexes();
        for(auto i: indexes) {
            if(i.isValid()) {
                auto item = model_->itemFromIndex(i);
                if(dynamic_cast<GroupedImages*>(item)) {
                    auto gr = reinterpret_cast<GroupedImages*>(item);
                    for(int ct = 0; ct < gr->rowCount(); ++ct) {
                        auto single = gr->child(ct);
                        single->setData(false, static_cast<int>(SingleImageItem::Role::ApprovedRole));
                        model_->dataChanged(single->index(), single->index());
                    }
                    gr->child(gr->curInFocus())->setData(true, static_cast<int>(SingleImageItem::Role::ApprovedRole));
                    model_->dataChanged(gr->child(gr->curInFocus())->index(), gr->child(gr->curInFocus())->index());
                    model_->dataChanged(i, i);
                }
            }
            else
                qDebug() << QString("Item is not valid");
            viewer_->updateLabels();
        }
    }));
    QShortcut *switchApprovRejectShC = new QShortcut(QKeySequence("R"), this);
    Q_ASSERT(connect(switchApprovRejectShC, &QShortcut::activated, [=](){
        auto indexes = dirView_->selectionModel()->selectedIndexes();
        for(auto i: indexes) {
            if(i.isValid()) {
                auto item = model_->itemFromIndex(i);
                if(dynamic_cast<GroupedImages*>(item)) {
                    auto gr = reinterpret_cast<GroupedImages*>(item);
                    for(int ct = 0; ct < gr->rowCount(); ++ct) {
                        auto single = gr->child(ct);
                        single->setData(false, static_cast<int>(SingleImageItem::Role::ApprovedRole));
                        model_->dataChanged(single->index(), single->index());
                    }
                    model_->dataChanged(i, i);
                }
            }
            else
                qDebug() << QString("Item is not valid");
            viewer_->updateLabels();
        }
    }));
    QShortcut *groupShC = new QShortcut(QKeySequence("G"), this);
    Q_ASSERT(connect(groupShC, &QShortcut::activated, [=](){
        auto indexes = dirView_->selectionModel()->selectedIndexes();
        QList<QStandardItem*> list;
        for(auto i: indexes) {
            if(i.isValid()) {
                auto item = model_->itemFromIndex(i);
                list << item;
            }
            else
                qDebug() << QString("Item is not valid");
        }
        if(list.size() > 1) model_->group(list);
    }));
    QShortcut *moveFocusLeft = new QShortcut(QKeySequence("S"), this);
    Q_ASSERT(connect(moveFocusLeft, &QShortcut::activated, viewer_, &Viewer::curToLeft));
    QShortcut *moveFocusRight = new QShortcut(QKeySequence("D"), this);
    Q_ASSERT(connect(moveFocusRight, &QShortcut::activated, viewer_, &Viewer::curToRight));
}

void MainWindow::onOpenRequest()
{
    QString dirName = QFileDialog::getExistingDirectory(this);
    if(!dirName.isEmpty()) {
        if(!model_->readCash(dirName))
            onOpenDir(dirName);
    }
}
