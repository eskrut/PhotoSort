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

};

#endif // DIRMODEL_H
