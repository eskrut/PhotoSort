#include "dirmodel.h"
#include "singleimageitem.h"
#include "groupedimages.h"
#include <QDebug>
#include <QMutex>

DirModel::DirModel(QObject *parent) :
    QStandardItemModel(parent),
    distanceThreashold_(15)
{
}

void DirModel::setup(const QStringList &entries)
{
    clear();
    int numWorkers = 0;
    const int maxWorkers = 20;
    QMutex mutex;
    for(auto e : entries) {
        auto item = new SingleImageItem(e);
        invisibleRootItem()->appendRow(item);
    }
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct));
        while(true) {
            mutex.lock();
            if(numWorkers >= maxWorkers) {
                mutex.unlock();
                thread()->yieldCurrentThread();
                continue;
            }
            else {
                mutex.unlock();
                break;
            }
        }
        LoadTask *task = new LoadTask(item, new QThread);
        Q_ASSERT(connect(task, &LoadTask::finished, this, &DirModel::emitDataChanged));
//        Q_ASSERT(connect(task, &LoadTask::started, [&](){
            mutex.lock();
            numWorkers++;
            mutex.unlock();
//        }));
        Q_ASSERT(connect(task, &LoadTask::finished, [&](){
            mutex.lock();
            numWorkers--;
            mutex.unlock();
        }));
        task->start();
    }
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct));
        while(true) {
            mutex.lock();
            if(numWorkers >= maxWorkers) {
                mutex.unlock();
                thread()->yieldCurrentThread();
                continue;
            }
            else {
                mutex.unlock();
                break;
            }
        }
        HashTask *task = new HashTask(item, new QThread);
//        Q_ASSERT(connect(task, &LoadTask::started, [&](){
            mutex.lock();
            numWorkers++;
            mutex.unlock();
//        }));
        Q_ASSERT(connect(task, &LoadTask::finished, [&](){
            mutex.lock();
            numWorkers--;
            mutex.unlock();
        }));
        task->start();
    }
    while(true) {
        mutex.lock();
        if(numWorkers > 0) {
            mutex.unlock();
            thread()->yieldCurrentThread();
            continue;
        }
        else {
            mutex.unlock();
            break;
        }
    }
    std::vector<std::vector<SingleImageItem*>> clusters;
    for(int ct = 0; ct < invisibleRootItem()->rowCount() - 1; ++ct) {
        clusters.push_back(std::vector<SingleImageItem*>({reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct))}));
//        qDebug() << invisibleRootItem()->child(ct)->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>() <<
        //                    invisibleRootItem()->child(ct+1)->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>() <<
        //                    ph_hamming_distance(invisibleRootItem()->child(ct)->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>(),
        //                                        invisibleRootItem()->child(ct+1)->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>());
    }
    //    bool modFlag = true;
    //    while(modFlag) {
    for(auto cl0 = clusters.begin(); cl0 < clusters.end(); ++cl0) {
        for(auto cl1 = cl0+1; cl1 < clusters.end(); ++cl1) {
            bool isSimilar = false;
            for(auto item0 : *cl0) {
                if(isSimilar) break;
                for(auto item1 : *cl1) {
                    int distance = ph_hamming_distance(item0->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>(),
                                                       item1->data(static_cast<int>(SingleImageItem::Role::GetHashRole)).value<ulong64>());
                    if(distance < distanceThreashold_) {
                        isSimilar = true;
                        break;
                    }
                }
            }
            if(isSimilar) {
                cl0->insert(cl0->end(), cl1->begin(), cl1->end());
                cl1->clear();
            }
        }
    }
    //    }
    QList<QStandardItem*> newItems;
    //clear();
    for(int ct = invisibleRootItem()->rowCount()-1; ct >=0; --ct)
        takeItem(ct);
    clear();
    for(auto &cl : clusters) {
        if(cl.size() == 1)
            newItems.push_back(cl.front());
        else if(cl.size() > 1) {
            GroupedImages *gr = new GroupedImages();
            QString text;
            for(auto i : cl) text += i->text() + ";";
            gr->setText(text);
            gr->setData(cl.front()->data(Qt::DecorationRole), Qt::DecorationRole);
            newItems.push_back(gr);
        }
    }
    for(auto i : newItems) invisibleRootItem()->appendRow(i);

    emitDataChanged();
}

void DirModel::emitDataChanged()
{
    emit dataChanged(QModelIndex(), QModelIndex());
    emit updateRequest();
}
