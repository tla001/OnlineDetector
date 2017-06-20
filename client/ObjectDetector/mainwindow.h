#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "streamdealthread.h"
#include "imagedealthread.h"
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void doPushStart();
    void doPushStop();
    void doPullStart();
    void doPullStop();

    void doTimeUpdate();
    void selectVideoPage();
    void selectImagePage();

    void doDispInfo(char *info);
    void doDispInfoWrite(char *info);
    void doDispInfoUpdate();


    void doOpenImage();
    void doPushImage();

private:
    Ui::MainWindow *ui;

    QTimer *timer;
    QTimer *timerInfo;
    QTimer *timerImageInfo;

    StreamDealThread pusher;
    StreamDealThread puller;

    string push_infile;
    string push_outfile;
    int push_fps;
    int push_bits;
    int push_gop;
    int push_disp;
    string push_win;

    string pull_infile;
    string pull_outfile;
    int pull_fps;
    int pull_bits;
    int pull_gop;
    int pull_disp;
    string pull_win;
    string infoStr;
    string infoStr1;
    string infoStrBak;

    bool runFlag;

    ImageDealThread imagepusher;
    string url;
    string filename;
    int timeout;

public:
    friend class StreamDeal;
    friend class ImageDeal;
};

#endif // MAINWINDOW_H
