#ifndef DIRMODEL_H
#define DIRMODEL_H

#include <QDir>
#include <QStandardItemModel>
#include "../threadpool/ThreadPool.h"
#include <memory>

class DirModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit DirModel(QObject *parent = 0);
    ~DirModel();
private:
    int distanceThreashold_;
    QDir curWorkDir_;
    Qt::ItemFlags curFlags_;
    std::unique_ptr<ThreadPool> pool_;
signals:
    void updateRequest();
    void selectionChanged(const QString &fileName);
    void loadProgress(int);
    void hashProgress(int);
    void loadDone();
    void hashDone();
    void done();
    void grouped(QStandardItem *);
public slots:
    void setup(const QStringList &entries, const QString &baseDir);
private slots:
    void emitDataChanged();
public:
//    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
//    bool removeRows(int row, int count, const QModelIndex &parent);

    void ungroup(QList<QStandardItem *> &list);
    void group(QList<QStandardItem *> &list);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
//    int rowCount(const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;
public slots:
    void process(const QString &dirName = QString("theBest"));
    void writeCache();
    void readCache(const QString &dir);
public:
    bool isCacheAvaileble(const QString &dir);
    int numImages();
    int numAccepted();

};

#endif // DIRMODEL_H
