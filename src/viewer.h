#ifndef VIEWER_H
#define VIEWER_H

#include <QStandardItem>
#include <QWidget>

class QLabel;
class QScrollArea;

class Viewer : public QWidget
{
    Q_OBJECT
public:
    explicit Viewer(QWidget *parent = 0);
private:
    QLabel *imageLabel_;
    QScrollArea *scrollArea_;
    double scaleFactor_;

signals:

public slots:
    void view(const QString &fileName);
    void viewItem(QStandardItem *item);

};

#endif // VIEWER_H
