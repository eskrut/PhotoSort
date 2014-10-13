#ifndef GROUPEDIMAGES_H
#define GROUPEDIMAGES_H

#include <QStandardItem>
#include "singleimageitem.h"

class GroupedImages : public QStandardItem
{
public:
    GroupedImages();
    QVariant data(int role) const;
    void setData(const QVariant &value, int role);

private:
    std::vector<int> outputIDs_;
    int curInFocus_;
public:
    std::vector<int> &outputIDs() {return outputIDs_;}
    int curInFocus() const;
    void setCurInFocus(int curInFocus);
};

#endif // GROUPEDIMAGES_H
