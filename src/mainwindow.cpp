#include "mainwindow.h"

#include "QBoxLayout"
#include "QHBoxLayout"
#include "QVBoxLayout"

#include <QDir>
#include <QDebug>
#include <QShortcut>
#include <QFileDialog>
#include <QThread>
#include <QKeySequence>
#include <QStringList>

#include "singleimageitem.h"
#include "groupedimages.h"
#include <functional>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QFrame *frame = new QFrame;
    auto lo = new QVBoxLayout(frame);
    auto lo1 = new QHBoxLayout();
    lo->addLayout(lo1);

    dirView_ = new DirView();
    lo1->addWidget(dirView_);
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
    lo1->addWidget(viewer_);
    viewer_->setMinimumWidth(200);

    progress_ = new QProgressBar(this);
    progress_->setTextVisible(true);
    lo->addWidget(progress_);

    setupSignals();
}

MainWindow::~MainWindow()
{
    delete model_;
}

void MainWindow::onOpenDir(const QString &dirName)
{
    QStringList entries;
    std::function<QStringList(const QString &dirName)> recurEntrues = nullptr;
    recurEntrues = [&](const QString &dirName){
        QDir dir(dirName);
        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QStringList entries;
        for(const auto &d : subDirs)
            entries << recurEntrues(dirName+"/"+d);
        for(const auto &e : dir.entryList(QStringList("*.JPG"))){
            entries << dirName + "/" + e;
        }
        return entries;
    };

    entries = recurEntrues(dirName);

    emit read(entries, dirName);
}

void MainWindow::setupSignals()
{
    QShortcut *openDirShC = new QShortcut(QKeySequence(QKeySequence::Open), this);
    connect(openDirShC, &QShortcut::activated, this, &MainWindow::onOpenRequest);

    QShortcut *processShC = new QShortcut(QKeySequence(QKeySequence::Print), this);
    connect(processShC, SIGNAL(activated()), model_, SLOT(process()));

    connect(model_, &DirModel::updateRequest, [=](){dirView_->update();});
    connect(this, &MainWindow::read, model_, &DirModel::setup);

    connect(dirView_->selectionModel(), &QItemSelectionModel::selectionChanged, [&](QItemSelection sel,QItemSelection desel){
                 auto indexes = sel.indexes();
                 if (indexes.size() == 1) {
                     viewer_->viewItem(model_->itemFromIndex(indexes.front()));
//                     viewer_->view(indexes.front().data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString());
//                     update();
                     viewer_->updateLabels();
                 }
             });
    QShortcut *switchApprovShC = new QShortcut(QKeySequence("A"), this);
    connect(switchApprovShC, &QShortcut::activated, [=](){
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
    });
    QShortcut *switchApprovForceShC = new QShortcut(QKeySequence("F"), this);
    connect(switchApprovForceShC, &QShortcut::activated, [=](){
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
    });
    QShortcut *switchApprovRejectShC = new QShortcut(QKeySequence("R"), this);
    connect(switchApprovRejectShC, &QShortcut::activated, [=](){
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
    });
    QShortcut *groupShC = new QShortcut(QKeySequence("G"), this);
    connect(groupShC, &QShortcut::activated, [=](){
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
        if(list.size() > 1) {
            model_->group(list);
            model_->writeCache();
        }
    });
    QShortcut *moveFocusLeft = new QShortcut(QKeySequence("S"), this);
    connect(moveFocusLeft, &QShortcut::activated, viewer_, &Viewer::curToLeft);
    QShortcut *moveFocusRight = new QShortcut(QKeySequence("D"), this);
    connect(moveFocusRight, &QShortcut::activated, viewer_, &Viewer::curToRight);

    connect(model_, &DirModel::loadProgress, progress_, &QProgressBar::setValue);
    connect(model_, &DirModel::hashProgress, progress_, &QProgressBar::setValue);

    progress_->setFormat(QString("%p"));
    connect(model_, &DirModel::loadDone, [=](){progress_->setFormat("%p%");});
    connect(model_, &DirModel::hashDone, [=](){progress_->setFormat("%p%");});
    connect(model_, &DirModel::done, [=](){progress_->setVisible(false);});
}

void MainWindow::onOpenRequest()
{
    QString dirName = QFileDialog::getExistingDirectory(this);
    if(!dirName.isEmpty()) {
        if(!model_->readCash(dirName))
            onOpenDir(dirName);
    }
}
