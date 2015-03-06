#include "singleimageitem.h"

#include <QDebug>
#include <QStyle>
#include <QtGui>

#include "libexif/exif-data.h"

SingleImageItem::SingleImageItem(const QString &fileName, const QString &rootDir) :
    QStandardItem(),
    fInfo_(fileName),
    hash_(0),
    rotation_(0),
    approved_(true)
{
    if(rootDir.isEmpty())
        setData(fInfo_.fileName(), Qt::DisplayRole);
    else
        setData(fInfo_.absoluteFilePath().replace(rootDir, ""), Qt::DisplayRole);
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
    case Qt::DecorationRole: {
        QPixmap p = img_;
        QPainter paint(&p);
        if(approved_)
            paint.drawPixmap(0, 0, 30, 30, QIcon(":/yes.ico").pixmap(30, 30));
        else
            paint.drawPixmap(0, 0, 30, 30, QIcon(":/no.ico").pixmap(30, 30));
        return p; }
        break;
    case static_cast<int>(Role::RotationRole):
        return rotation_;
        break;
    case static_cast<int>(Role::ApprovedRole):
        return approved_;
        break;
    case static_cast<int>(SingleImageItem::Role::GroupedFiles):
        return QStringList(data(Qt::DisplayRole).toString());
        break;
    default:
        return QStandardItem::data(role);
    }
}

void SingleImageItem::setData(const QVariant &value, int role)
{
    emitDataChanged();
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
    case static_cast<int>(Role::ApprovedRole):
        approved_ = value.value<bool>();
        break;
    default:
        QStandardItem::setData(value, role);
    }
}

int SingleImageItem::load()
{
    QString fileName = data(static_cast<int>(SingleImageItem::Role::FileNameRole)).toString();
    ExifData *ed = exif_data_new_from_file(fileName.toLocal8Bit());
    ExifEntry *entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
    ExifByteOrder byte_order;
    byte_order = exif_data_get_byte_order (ed);
    auto orient = exif_get_short(entry->data, byte_order);

    QPixmap img;
    img.load(fileName);
    img = img.scaled(250, 250, Qt::KeepAspectRatio);
    int rot = 0;
    if(orient == 8) rot = -90;
    else if(orient == 6) rot = 90;
    else if(orient == 3) rot = 180;
    else if(orient != 1)qDebug() << QString("Unknown orientation %1").arg(orient);
    if( rot != 0 ){
        QTransform t;
        t.rotate(rot);
        img = img.transformed(t);
        setData(rot, static_cast<int>(SingleImageItem::Role::RotationRole));
    }

    setData(img, Qt::DecorationRole);
    exif_data_free(ed);
    return row();
}
