#include "groupedimages.h"

GroupedImages::GroupedImages():
    QStandardItem()
{
    outputIDs_.push_back(0);
}

QVariant GroupedImages::data(int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if(outputIDs_.size() && rowCount())
            return child(outputIDs_.front())->data(Qt::DecorationRole);
        else
            return QVariant();
        break;
    default:
        return QStandardItem::data(role);
    }
}

QString GroupedImages::toolTip() const
{
    return QString("%1 Images").arg(rowCount());
}
