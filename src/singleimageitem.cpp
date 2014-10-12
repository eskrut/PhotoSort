#include "singleimageitem.h"

#include <QDebug>
#include <QStyle>

#include "libexif/exif-data.h"

SingleImageItem::SingleImageItem(const QString &fileName) :
    QStandardItem(),
    fInfo_(fileName),
    hash_(0),
    rotation_(0)
{
    qDebug() << fInfo_.absoluteFilePath();
    setData(fInfo_.fileName(), Qt::DisplayRole);
    img_ = QIcon(":/load.png").pixmap(250, 250);
}

QVariant SingleImageItem::data(int role) const
{
    switch (role) {
    case static_cast<int>(Role::GetHashRole):
        return QVariant::fromValue(hash_);
        break;
    case static_cast<int>(Role::FileNameRole):
        return fInfo_.absoluteFilePath();
        break;
    case Qt::DecorationRole:
        return img_;
        break; 
    case static_cast<int>(Role::RotationRole):
        return rotation_;
        break;
    default:
        return QStandardItem::data(role);
    }
}

void SingleImageItem::setData(const QVariant &value, int role)
{
    switch (role) {
    case static_cast<int>(Role::GetHashRole):
        hash_ = value.value<ulong64>();
        break;
    case Qt::DecorationRole:
        img_ = value.value<QPixmap>();
        break;
    case static_cast<int>(Role::RotationRole):
        rotation_ = value.value<int>();
        break;
    default:
        QStandardItem::setData(value, role);
    }
}

void SingleImageItem::load()
{
    LoadTask *task = new LoadTask(this, new QThread);
    Q_ASSERT(QObject::connect(task, &LoadTask::finished, [=](){this->emitDataChanged();}));
    task->start();
}

void LoadTask::run()
{
    QString fileName = item_->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
    ExifData *ed = exif_data_new_from_file(fileName.toLocal8Bit());
    ExifEntry *entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
    ExifByteOrder byte_order;
    byte_order = exif_data_get_byte_order (ed);
    auto orient = exif_get_short(entry->data, byte_order);

    QPixmap img;
    img.load(fileName);
    img = img.scaled(250, 250, Qt::KeepAspectRatio);
    if(orient > 1) {
        QTransform t;
        t.rotate(-90);
        img = img.transformed(t);
        item_->setData(-90, static_cast<int>(SingleImageItem::Role::RotationRole));
    }

    item_->setData(img, Qt::DecorationRole);
    exif_data_free(ed);
}


void HashTask::run()
{
    ulong64 hash;
    QString fileName = item_->data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
    if( ph_dct_imagehash(fileName.toLocal8Bit(), hash) >= 0)
        item_->setData(hash, static_cast<int>(SingleImageItem::Role::GetHashRole));
}
