#ifndef DIRMODEL_H
#define DIRMODEL_H

#include <QStandardItemModel>

class DirModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit DirModel(QObject *parent = 0);
private:
    int distanceThreashold_;
signals:
    void updateRequest();
    void selectionChanged(const QString &fileName);
public slots:
    void setup(const QStringList &entries);
private slots:
    void emitDataChanged();
public:
//    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
//    bool removeRows(int row, int count, const QModelIndex &parent);

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
//    int rowCount(const QModelIndex &parent) const;
    Qt::DropActions supportedDropActions() const;

};

#endif // DIRMODEL_H
