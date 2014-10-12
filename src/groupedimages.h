#ifndef GROUPEDIMAGES_H
#define GROUPEDIMAGES_H

#include <QStandardItem>
#include "singleimageitem.h"

class GroupedImages : public QStandardItem
{
public:
    GroupedImages();
    QVariant data(int role) const;
private:
    std::vector<int> outputIDs_;
};

#endif // GROUPEDIMAGES_H
