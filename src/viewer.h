#ifndef VIEWER_H
#define VIEWER_H

#include <QStandardItem>
#include <QWidget>

class QLabel;
class QScrollArea;
class SingleImageItem;
class GroupedImages;

class Viewer : public QWidget
{
    Q_OBJECT
public:
    explicit Viewer(QWidget *parent = 0);
private:
    QLabel *imageLabel_;
    QScrollArea *scrollArea_;
    QScrollArea *miniaturesScroll_;
    QList<QLabel*> labels_;
    double scaleFactor_;
    GroupedImages *curGr_;
    QStandardItem *curItem_;

signals:

public slots:
    void view(const QString &fileName);
    void viewItem(QStandardItem *item);
    void updateLabels();
    void curToRight();
    void curToLeft();
private slots:
    void viewGroup(GroupedImages *gr);
private:
    void activateImage(QStandardItem *item);
};

#endif // VIEWER_H
