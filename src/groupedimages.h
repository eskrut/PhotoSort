#ifndef GROUPEDIMAGES_H
#define GROUPEDIMAGES_H

#include <QStandardItem>
#include "singleimageitem.h"

class GroupedImages : public QStandardItem
{
public:
    GroupedImages();
    QVariant data(int role) const;
    enum class Role{
        NumSingles = Qt::UserRole + 1
    };

private:
    std::vector<int> outputIDs_;
public:
    QString toolTip() const;
};

#endif // GROUPEDIMAGES_H
