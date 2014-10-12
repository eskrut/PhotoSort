#include "viewer.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include "singleimageitem.h"
#include "groupedimages.h"

Viewer::Viewer(QWidget *parent) :
    QWidget(parent)
{
    auto lo = new QVBoxLayout;
    imageLabel_ = new QLabel;
    imageLabel_->setBackgroundRole(QPalette::Base);
    imageLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel_->setScaledContents(true);

    scrollArea_ = new QScrollArea;
    scrollArea_->setBackgroundRole(QPalette::Dark);
    scrollArea_->setWidget(imageLabel_);

    lo->addWidget(scrollArea_);
    this->setLayout(lo);
}

void Viewer::view(const QString &fileName)
{
    QImage image(fileName);
    imageLabel_->setPixmap(QPixmap::fromImage(image));
    scaleFactor_ = 1.0;
    imageLabel_->adjustSize();
}

void Viewer::viewItem(QStandardItem *item)
{
    QString fileName = item->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
    QImage image(fileName);
    if(!image.isNull()) {
        bool ok;
        int rot = item->data(static_cast<int>(SingleImageItem::Role::RotationRole)).toInt(&ok);
        if(ok) {
            QTransform t;
            t.rotate(rot);
            image = image.transformed(t);
        }
    }
//        imageLabel_->setPixmap(QPixmap::fromImage(image));
        auto pm = QPixmap::fromImage(image);
        imageLabel_->setPixmap(pm.scaled(scrollArea_->size(), Qt::KeepAspectRatio));
        imageLabel_->setAlignment(Qt::AlignCenter);
        scaleFactor_ = 1.0;
        imageLabel_->adjustSize();
        scrollArea_->setWidgetResizable(false);
}
