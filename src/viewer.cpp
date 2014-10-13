#include "viewer.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <qpainter.h>
#include "singleimageitem.h"
#include "groupedimages.h"

Viewer::Viewer(QWidget *parent) :
    QWidget(parent),
    curGr_(nullptr)
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

    auto miniaturesLo = new QHBoxLayout;
//    lo->addLayout(miniaturesLo);
    miniaturesScroll_ = new QScrollArea;
    miniaturesScroll_->setLayout(miniaturesLo);
    miniaturesScroll_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    miniaturesScroll_->setWidgetResizable(false);
    miniaturesScroll_->setMinimumHeight(250);
    lo->addWidget(miniaturesScroll_);
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
    curGr_ = nullptr;
    curItem_ = item;
    activateImage(item);
}

void Viewer::updateLabels()
{
    if(curGr_){
        const int numImgs = curGr_->rowCount();
        for(int ct = 0; ct < numImgs; ++ct) {
            labels_[ct]->show();
            auto pm = curGr_->child(ct)->data(Qt::DecorationRole).value<QPixmap>().scaled(200, 200, Qt::KeepAspectRatio);
            labels_[ct]->setPixmap(pm);
            labels_[ct]->setAlignment(Qt::AlignCenter);
        }
    }
    else
        for(int ct = 0; ct < labels_.size(); ++ct) labels_[ct]->hide();
}

void Viewer::curToRight()
{
    if(curGr_) {
            labels_[curGr_->curInFocus()]->setFrameStyle(QFrame::NoFrame);
            curGr_->setCurInFocus((curGr_->curInFocus()+1) % curGr_->rowCount());
            labels_[curGr_->curInFocus()]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
            activateImage(curGr_->child(curGr_->curInFocus()));
    }
}

void Viewer::curToLeft()
{
    if(curGr_) {
        labels_[curGr_->curInFocus()]->setFrameStyle(QFrame::NoFrame);
        int newID = curGr_->curInFocus()-1;
        newID = newID > 0 ? newID : curGr_->rowCount() - 1;
        curGr_->setCurInFocus(newID % curGr_->rowCount());
        labels_[curGr_->curInFocus()]->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        activateImage(curGr_->child(curGr_->curInFocus()));
    }
}

void Viewer::viewGroup(GroupedImages *gr)
{
    curGr_ = gr;
    miniaturesScroll_->show();
    const int numImgs = gr->rowCount();
    for(int ct = numImgs; ct < labels_.size(); ++ct) labels_[ct]->hide();
    for(int ct = labels_.size(); ct < numImgs; ++ct) {
        labels_.push_back(new QLabel);
//        labels_.back()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        miniaturesScroll_->layout()->addWidget(labels_.back());
    }
    for(int ct = 0; ct < numImgs; ++ct) {
        labels_[ct]->show();
        auto pm = gr->child(ct)->data(Qt::DecorationRole).value<QPixmap>().scaled(200, 200, Qt::KeepAspectRatio);
        labels_[ct]->setPixmap(pm);
        labels_[ct]->setAlignment(Qt::AlignCenter);
        labels_[ct]->setFrameStyle(QFrame::NoFrame);
    }
    labels_.front()->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    gr->setCurInFocus(0);
}

void Viewer::activateImage(QStandardItem *item)
{
    curItem_ = item;
    SingleImageItem *single = dynamic_cast<SingleImageItem*>(item);
    GroupedImages *gr;
    QString fileName;
    QImage image;
    if(!single) {
        gr = dynamic_cast<GroupedImages*>(item);
        viewGroup(gr);
        if(gr->outputIDs().size())
            single = reinterpret_cast<SingleImageItem*>(gr->child(gr->outputIDs().front()));
    }
    if(single) {
        fileName = single->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
        image = QImage(fileName);
        if(!image.isNull()) {
            bool ok;
            int rot = single->data(static_cast<int>(SingleImageItem::Role::RotationRole)).toInt(&ok);
            if(ok) {
                QTransform t;
                t.rotate(rot);
                image = image.transformed(t);
            }
        }
    }
    auto pm = QPixmap::fromImage(image);
    pm = pm.scaled(scrollArea_->size(), Qt::KeepAspectRatio);
    QPainter painter(&pm);
    if(item->data(static_cast<int>(SingleImageItem::Role::ApprovedRole)).value<bool>())
        painter.drawPixmap(0, 0, 30, 30, QIcon(":/yes.ico").pixmap(30, 30));
    else
        painter.drawPixmap(0, 0, 30, 30, QIcon(":/no.ico").pixmap(30, 30));
    imageLabel_->setPixmap(pm);
    imageLabel_->setAlignment(Qt::AlignCenter);
    scaleFactor_ = 1.0;
    imageLabel_->adjustSize();
    scrollArea_->setWidgetResizable(false);
}
