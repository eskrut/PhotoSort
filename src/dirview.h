#ifndef DIRVIEW_H
#define DIRVIEW_H

#include <QListView>

class DirView : public QListView
{
    Q_OBJECT
public:
    explicit DirView(QWidget *parent = 0);
private:
    void keyPressEvent(QKeyEvent *event);
signals:

public slots:

};

#endif // DIRVIEW_H
