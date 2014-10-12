#include "dirmodel.h"
#include "singleimageitem.h"
#include "groupedimages.h"
#include <QDebug>
#include <QMutex>
#include <QMimeData>

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
            for(auto i : cl) {
                text += i->text() + ";";
                gr->appendRow(i);
            }
            gr->setText(text);
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

Qt::ItemFlags DirModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool DirModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasFormat("image/sort-data"))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    if (!parent.isValid())
        return false;

    SingleImageItem *sParent = dynamic_cast<SingleImageItem*>(itemFromIndex(parent));
    GroupedImages *gParent = dynamic_cast<GroupedImages*>(itemFromIndex(parent));
    Q_ASSERT(sParent || gParent);

    QByteArray encodedData = data->data("image/sort-data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    qDebug() << QString("Trying to decode") << row << column << parent.data();
    while (!stream.atEnd()) {
        QStandardItem *item = nullptr;
        quintptr ptr;
        stream >> ptr;
        item = reinterpret_cast<QStandardItem*>(ptr);
        SingleImageItem *single = dynamic_cast<SingleImageItem*>(item);
        if(single != nullptr) {
            takeItem(item->row(), item->column());
            if(gParent) {
                gParent->appendRow(single);
                gParent->setText(gParent->text() + ";" + single->text());
            }
            else {
                gParent = new GroupedImages;
                gParent->setText(sParent->text());
                int row = sParent->row();
                gParent->appendRow(takeItem(sParent->row(), sParent->column()));
                gParent->appendRow(single);
                gParent->setText(gParent->text() + ";" + single->text());
                invisibleRootItem()->insertRow(row, gParent);
                sParent = nullptr;
            }
        }
        else {
            GroupedImages *group = dynamic_cast<GroupedImages*>(item);
            for(int ct = group->rowCount() - 1; ct >= 0; --ct) {
                auto single = group->takeChild(ct, 0);
                if(gParent) {
                    gParent->appendRow(single);
                    gParent->setText(gParent->text() + ";" + single->text());
                }
                else {
                    gParent = new GroupedImages;
                    gParent->setText(sParent->text());
                    int row = sParent->row();
                    gParent->appendRow(takeItem(sParent->row(), sParent->column()));
                    gParent->appendRow(single);
                    gParent->setText(gParent->text() + ";" + single->text());
                    invisibleRootItem()->insertRow(row, gParent);
                    sParent = nullptr;
                }
            }
            takeItem(group->row(), group->column());
        }
    }

    sort(0);
    emitDataChanged();

    return true;
}

QMimeData *DirModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            quintptr ptr(reinterpret_cast<quintptr>(itemFromIndex(index)));
            stream << ptr;
            qDebug() << ptr;
        }
    }
    qDebug() << QString("Data encoded");

    mimeData->setData("image/sort-data", encodedData);
    return mimeData;
}

QStringList DirModel::mimeTypes() const
{
    QStringList types;
    types << "image/sort-data";
    return types;
}

Qt::DropActions DirModel::supportedDropActions() const
{
    return /*Qt::MoveAction | */Qt::CopyAction;
}
