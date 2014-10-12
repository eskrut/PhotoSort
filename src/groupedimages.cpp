#include "groupedimages.h"

GroupedImages::GroupedImages():
    QStandardItem()
{
}

QVariant GroupedImages::data(int role) const
{
    switch (role) {
    default:
        return QStandardItem::data(role);
    }
}
