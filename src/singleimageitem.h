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
    explicit SingleImageItem(const QString &fileName, const QString &rootDir = QString());
    enum class Role {
        GetHashRole = Qt::UserRole + 1,
        FileNameRole,
        RotationRole,
        ApprovedRole,
        GroupedFiles
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
    int load();
};

#endif // SINGLEIMAGEITEM_H
