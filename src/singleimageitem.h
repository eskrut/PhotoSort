#ifndef SINGLEIMAGEITEM_H
#define SINGLEIMAGEITEM_H

#include <QStandardItem>
#include <QFileInfo>
#include <QPixmap>
#include <QThread>
#include "pHash.h"

class SingleImageItem : public QStandardItem
{
public:
    explicit SingleImageItem(const QString &fileName);
    enum class Role {
        GetHashRole = Qt::UserRole + 1,
        FileNameRole,
        RotationRole,
        ApprovedRole
    };
private:
    QFileInfo fInfo_;
    ulong64 hash_;
    QPixmap img_;
    int rotation_;
    bool approved_;
public:
    QVariant data(int role) const;
    void setData(const QVariant &value, int role);
    void load();
};

class LoadTask : public QThread
{
    Q_OBJECT
public:
    LoadTask(SingleImageItem *item, QObject *parent = nullptr) : QThread(parent), item_(item) {}
private:
    SingleImageItem *item_;
    void run() Q_DECL_OVERRIDE;
};

class HashTask : public QThread
{
    Q_OBJECT
public:
    HashTask(SingleImageItem *item, QObject *parent = nullptr) : QThread(parent), item_(item) {}
private:
    SingleImageItem *item_;
    void run() Q_DECL_OVERRIDE;
};

#endif // SINGLEIMAGEITEM_H
