#include "groupedimages.h"
#include <QDebug>
#include <QStringList>

GroupedImages::GroupedImages():
    QStandardItem()
{
    outputIDs_.push_back(0);
    curInFocus_ = 0;
}

QVariant GroupedImages::data(int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        return QString("%1 + %2").arg(child(0)->text()).arg(rowCount()-1);
        break;
    case Qt::DecorationRole:
        if(outputIDs_.size() && rowCount())
            return child(outputIDs_.front())->data(Qt::DecorationRole);
        else
            return QVariant();
        break;
    case static_cast<int>(SingleImageItem::Role::ApprovedRole):
        return child(curInFocus_)->data(static_cast<int>(SingleImageItem::Role::ApprovedRole));
        break;
    case static_cast<int>(SingleImageItem::Role::GroupedFiles):
    {
        QStringList files;
        for(int ct = 0; ct < rowCount();++ct) files << child(ct)->data(static_cast<int>(SingleImageItem::Role::GroupedFiles)).toStringList();
        return files;
        break;
    }
    default:
        return QStandardItem::data(role);
    }
}

void GroupedImages::setData(const QVariant &value, int role)
{
    switch (role) {
    case static_cast<int>(SingleImageItem::Role::ApprovedRole):
        child(curInFocus_)->setData(value, static_cast<int>(SingleImageItem::Role::ApprovedRole));
        break;
    default:
        QStandardItem::setData(value, role);
    }
}

int GroupedImages::curInFocus() const
{
    return curInFocus_;
}

void GroupedImages::setCurInFocus(int curInFocus)
{
    curInFocus_ = curInFocus;
}


