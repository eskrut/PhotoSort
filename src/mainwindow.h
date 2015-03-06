#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListView>
#include <QMainWindow>
#include "dirmodel.h"
#include "dirview.h"
#include "viewer.h"
#include <QProgressBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    DirView *dirView_;
    DirModel *model_;
    Viewer *viewer_;
    QProgressBar *progress_;

signals:
    void read(const QStringList &entries, const QString &baseDir);

public slots:
    void onOpenDir(const QString &dirNAme);
private:
    void setupSignals();
private slots:
    void onOpenRequest();
};

#endif // MAINWINDOW_H
