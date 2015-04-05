#include "dirmodel.h"
#include "singleimageitem.h"
#include "groupedimages.h"
#include <QDebug>
#include <QMutex>
#include <QMimeData>
#include <QString>
#include <QTransform>
#include <QPersistentModelIndex>
#include <list>
#include <atomic>
#include <QFile>
#include "libexif/exif-data.h"

DirModel::DirModel(QObject *parent) :
    QStandardItemModel(parent),
    distanceThreashold_(5),
    pool_(new ThreadPool(8))
{
    curFlags_ = Qt::NoItemFlags;
}

DirModel::~DirModel()
{
    writeCache();
}

void DirModel::setup(const QStringList &entries, const QString &baseDir)
{
    curWorkDir_ = baseDir;
    clear();
    for(auto e : entries) {
        auto item = new SingleImageItem(e, curWorkDir_.absolutePath());
        invisibleRootItem()->appendRow(item);
    }
    std::list<std::future<int>> loadFutures, hashFutures;
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct));
        auto load = [item](){
            return item->load();
        };
        loadFutures.push_back(pool_->enqueue(load));
    }
    std::atomic<int> progress(0);
    for(auto && f : loadFutures) {
        f.get();
        progress++;
        loadProgress(progress*100/invisibleRootItem()->rowCount());
    }
    emit(loadDone());
    qDebug() << "Load OK";
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct));
        auto hash = [item](){
            ulong64 hash;
            QString fileName = item->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
            if( ph_dct_imagehash(fileName.toLocal8Bit(), hash) >= 0)
                item->setData(hash, static_cast<int>(SingleImageItem::Role::GetHashRole));
            return item->row();
        };
        hashFutures.push_back(pool_->enqueue(hash));
    }
    progress = 0;
    for(auto && f : hashFutures) {
        f.get();
        progress++;
        hashProgress(progress*100/invisibleRootItem()->rowCount());
    }
    emit(hashDone());
    qDebug() << "Hashing OK";
    std::vector<std::vector<SingleImageItem*>> clusters;
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct)
        clusters.push_back(std::vector<SingleImageItem*>({reinterpret_cast<SingleImageItem*>(invisibleRootItem()->child(ct))}));
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
    qDebug() << "Grouping OK";
    QList<QStandardItem*> newItems;
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
                i->setData(false, static_cast<int>(SingleImageItem::Role::ApprovedRole));
                text += i->text() + ";";
                gr->appendRow(i);
            }
            cl.front()->setData(true, static_cast<int>(SingleImageItem::Role::ApprovedRole));
            gr->setText(text);
            newItems.push_back(gr);
        }
    }
    for(auto i : newItems) invisibleRootItem()->appendRow(i);

    curFlags_ = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    emitDataChanged();
    emit(done());
    emit(grouped(nullptr));
}

void DirModel::emitDataChanged()
{
    emit dataChanged(QModelIndex(), QModelIndex());
    emit updateRequest();
}

Qt::ItemFlags DirModel::flags(const QModelIndex &index) const
{
    return curFlags_;
//    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    //    return index.flags();
}

void DirModel::ungroup(QList<QStandardItem *> &list)
{

}

void DirModel::group(QList<QStandardItem *> &list)
{
    for(auto item : list) qDebug() << item->data(static_cast<int>(SingleImageItem::Role::GroupedFiles));
    QList<QPersistentModelIndex> indexes;
    for(auto item : list) indexes << QPersistentModelIndex(item->index());

    QStandardItem *first = list.takeFirst();
    SingleImageItem *sParent = dynamic_cast<SingleImageItem*>(first);
    GroupedImages *gParent = dynamic_cast<GroupedImages*>(first);
    Q_ASSERT(sParent || gParent);
    for(auto item : list) {
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
    qDebug() << gParent->data(static_cast<int>(SingleImageItem::Role::GroupedFiles));
    emitDataChanged();
    emit(grouped(gParent));
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

    QList<QStandardItem*> list;
    list << itemFromIndex(parent);

    QByteArray encodedData = data->data("image/sort-data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        QStandardItem *item = nullptr;
        quintptr ptr;
        stream >> ptr;
        item = reinterpret_cast<QStandardItem*>(ptr);
        list << item;
    }
    group(list);

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

void DirModel::process(const QString &dirName)
{
    if(!curWorkDir_.mkdir(dirName)) return;
    QStringList filesToCopy;
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = invisibleRootItem()->child(ct);
        SingleImageItem *single = dynamic_cast<SingleImageItem*>(item);
        if(single) {
            if(single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool())
                filesToCopy << single->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
        }
        else {
            GroupedImages *gr = dynamic_cast<GroupedImages*>(item);
            if(gr) {
                for(int childCt = 0; childCt < gr->rowCount(); ++childCt) {
                    auto single = gr->child(childCt);
                    if(single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool())
                        filesToCopy << single->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
                }
            }
        }
    }
    filesToCopy.sort();
    int count = 1;
    for(auto f : filesToCopy) {
        QFileInfo fInfo(f);
        QString newName = curWorkDir_.absolutePath() + "/"+dirName+"/" + QString("%1_").arg(count++, 5, 10, QChar('0')) + fInfo.fileName();
        QFile::copy(f, newName);
    }
}

void DirModel::writeCache()
{
    QFile cash(curWorkDir_.absolutePath()+"/cache");
    cash.open(QIODevice::WriteOnly);
    QTextStream out(&cash);
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = invisibleRootItem()->child(ct);
        SingleImageItem *single = dynamic_cast<SingleImageItem*>(item);
        if(single) {
            QString line;
            QString rec = single->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
            rec = rec.replace(curWorkDir_.absolutePath(), "");
            line = line.append(rec).append("|").append("%1|").arg(single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool());
            out << line << endl;
        }
        else {
            GroupedImages *gr = dynamic_cast<GroupedImages*>(item);
            if(gr) {
                QString line;
                for(int childCt = 0; childCt < gr->rowCount(); ++childCt) {
                    SingleImageItem* single = reinterpret_cast<SingleImageItem*>(gr->child(childCt));
                    QString rec = single->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
                    rec = rec.replace(curWorkDir_.absolutePath(), "");
                    line = line.append(rec).append("|").append("%1|").arg(single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool());
                }
                out << line << endl;
            }
        }
    }
    cash.close();
}

bool DirModel::isCacheAvaileble(const QString &dir)
{
    QFile cache(QDir(dir).absolutePath()+"/cache");
    return cache.exists();
}

int DirModel::numImages()
{
    int num = 0;
    for(int ct = 0; ct < rowCount(); ++ct){
        int subNum = item(ct, 0)->rowCount();
        num += subNum > 1 ? subNum : 1;
    }
    return num;
}

int DirModel::numAccepted()
{
    int num = 0;
    for(int ct = 0; ct < invisibleRootItem()->rowCount(); ++ct) {
        auto item = invisibleRootItem()->child(ct);
        SingleImageItem *single = dynamic_cast<SingleImageItem*>(item);
        if(single && single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool())
            num++;
        else {
            GroupedImages *gr = dynamic_cast<GroupedImages*>(item);
            if(gr) {
                for(int childCt = 0; childCt < gr->rowCount(); ++childCt) {
                    SingleImageItem* single = reinterpret_cast<SingleImageItem*>(gr->child(childCt));
                    if(single->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).toBool())
                        num++;
                }
            }
        }
    }
    return num;
}

void DirModel::readCache(const QString &dir)
{
    curWorkDir_ = QDir(dir);
    QFile cache(curWorkDir_.absolutePath()+"/cache");
    if(cache.open(QIODevice::ReadOnly)) {
        QTextStream in(&cache);
        QString line;
        std::list<std::future<int>> loadFutures;
        do {
            line = in.readLine();
            auto items = line.split("|", QString::SkipEmptyParts);
            if(items.size() == 2) {
                SingleImageItem *single = new SingleImageItem(curWorkDir_.absolutePath()+"/"+items[0], curWorkDir_.absolutePath());
                //single->load();
                auto load = [single](){
                    return single->load();
                };
                loadFutures.push_back(pool_->enqueue(load));
                single->setData(items[1].toInt(), static_cast<int>(SingleImageItem::Role::ApprovedRole));
                invisibleRootItem()->appendRow(single);
            }
            else if(items.size() > 2){
                GroupedImages *gr = new GroupedImages;
                for(int ct = 0; ct < items.size(); ct += 2){
                    SingleImageItem *single = new SingleImageItem(curWorkDir_.absolutePath()+"/"+items[ct], curWorkDir_.absolutePath());
                    //single->load();
                    auto load = [single](){
                        return single->load();
                    };
                    loadFutures.push_back(pool_->enqueue(load));
                    single->setData(items[ct+1].toInt(), static_cast<int>(SingleImageItem::Role::ApprovedRole));
                    gr->appendRow(single);
                }
                invisibleRootItem()->appendRow(gr);
            }
        } while (!line.isNull());
        std::atomic<int> progress(0);
        int numAll = numImages();
        for(auto && f : loadFutures) {
            f.get();
            progress++;
            loadProgress(progress*100/numAll);
        }
        emit(loadDone());
        qDebug() << "Load OK";
        curFlags_ = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        emitDataChanged();
        emit(done());
        emit(grouped(nullptr));
    }
}
