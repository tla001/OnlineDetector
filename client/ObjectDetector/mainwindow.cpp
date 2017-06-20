#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextCursor>
#include <QFileDialog>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Online Detector");
    setWindowIcon(QIcon(":/image/resource/detector.ico"));

    push_infile="/dev/video0";
    push_outfile="rtmp://219.216.87.170/live/test1";
    push_fps=30;
    push_bits=700000;
    push_gop=5;
    push_disp=1;
    push_win="Pusher";

    pull_infile="rtmp://219.216.87.170/live/test2";
    pull_outfile="test.flv";
    pull_fps=30;
    pull_bits=700000;
    pull_gop=5;
    pull_disp=1;
    pull_win="Puller";
    runFlag=false;
    infoStr="";
    infoStr1="";
    infoStrBak="";

    url="219.216.87.170:5555/test";
    filename="";
    timeout=10;


    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(doTimeUpdate()));
    timer->start(1000);

    ui->Edit_pushInfile->setText(push_infile.c_str());
    ui->Edit_pushOutfile->setText(push_outfile.c_str());
    ui->Edit_pushFps->setText(QString::number(push_fps));
    ui->Edit_pushBits->setText(QString::number(push_bits));
    ui->Edit_pushGop->setText(QString::number(push_gop));
    ui->Edit_pushWin->setText(push_win.c_str());
    ui->Button_pushDISP->setChecked(true);
    ui->Edit_pullInfile->setText(pull_infile.c_str());
    ui->Edit_pullOutfile->setText(pull_outfile.c_str());
    ui->Edit_pullFps->setText(QString::number(pull_fps));
    ui->Edit_pullBits->setText(QString::number(pull_bits));
    ui->Edit_pullGop->setText(QString::number(pull_gop));
    ui->Edit_pullWin->setText(pull_win.c_str());
    ui->Button_pullDISP->setChecked(true);

    ui->Button_pullStop->setEnabled(false);
    ui->Button_pushStop->setEnabled(false);
    connect(ui->Button_pushStart,SIGNAL(clicked(bool)),this,SLOT(doPushStart()));
    connect(ui->Button_pushStop,SIGNAL(clicked(bool)),this,SLOT(doPushStop()));
    connect(ui->Button_pullStart,SIGNAL(clicked(bool)),this,SLOT(doPullStart()));
    connect(ui->Button_pullStop,SIGNAL(clicked(bool)),this,SLOT(doPullStop()));
    connect(ui->Button_image,SIGNAL(clicked(bool)),this,SLOT(selectImagePage()));
    connect(ui->Button_video,SIGNAL(clicked(bool)),this,SLOT(selectVideoPage()));

    timerInfo=new QTimer(this);
    connect(timerInfo,SIGNAL(timeout()),this,SLOT(doDispInfoUpdate()));
    timerImageInfo=new QTimer(this);
    connect(timerImageInfo,SIGNAL(timeout()),this,SLOT(doDispInfoUpdate()));

    ui->Edit_imagePath->setText(filename.c_str());
    ui->Edit_URL->setText(url.c_str());
    connect(ui->Button_openImage,SIGNAL(clicked(bool)),this,SLOT(doOpenImage()));
    connect(ui->Button_excute,SIGNAL(clicked(bool)),this,SLOT(doPushImage()));


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::doPushStart(){
//    push_infile="/dev/video0";
//    push_outfile="rtmp://219.216.87.170/live/test1";
    if(ui->Edit_pushInfile->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushInfile->setFocus();
       return;
    }
    if(ui->Edit_pushOutfile->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushOutfile->setFocus();
       return;
    }
    if(ui->Edit_pushFps->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushFps->setFocus();
       return;
    }
    if(ui->Edit_pushBits->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushBits->setFocus();
       return;
    }
    if(ui->Edit_pushGop->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushGop->setFocus();
       return;
    }
    if(ui->Edit_pushWin->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pushWin->setFocus();
       return;
    }
    push_infile=ui->Edit_pushInfile->text().toLocal8Bit().data();
    push_outfile=ui->Edit_pushOutfile->text().toLocal8Bit().data();
    push_win=ui->Edit_pushWin->text().toLocal8Bit().data();
    push_bits=ui->Edit_pushBits->text().toInt();
    push_fps=ui->Edit_pullFps->text().toInt();
    push_gop=ui->Edit_pushGop->text().toInt();
    if(ui->Button_pushDISP->isChecked())
        push_disp=1;
    else
        push_disp=0;
    pusher.init(const_cast<char*>(push_infile.c_str()),const_cast<char*>(push_outfile.c_str()),
                push_fps,push_bits,push_gop,push_disp,const_cast<char*>(push_win.c_str()));
    pusher.setId(1);
    pusher.start();

    ui->Button_pushStart->setEnabled(false);
    ui->Button_pushStop->setEnabled(true);
    runFlag=true;

    infoStr="1push start!";
    doDispInfo(const_cast<char*>(infoStr.c_str()));
    timerInfo->start(100);
}
void MainWindow::doPushStop(){
    pusher.doStop();
    ui->Button_pushStart->setEnabled(true);
    ui->Button_pushStop->setEnabled(false);
    runFlag=false;

    timerInfo->stop();
    infoStr="1push stop!";
    doDispInfo(const_cast<char*>(infoStr.c_str()));

}
void MainWindow::doPullStart(){
//    pull_infile="rtmp://219.216.87.170/live/test2";
//    pull_outfile="test.flv";
    if(ui->Edit_pullInfile->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullInfile->setFocus();
       return;
    }
    if(ui->Edit_pullOutfile->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullOutfile->setFocus();
       return;
    }
    if(ui->Edit_pullFps->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullFps->setFocus();
       return;
    }
    if(ui->Edit_pullBits->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullBits->setFocus();
       return;
    }
    if(ui->Edit_pullGop->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullGop->setFocus();
       return;
    }
    if(ui->Edit_pullWin->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_pullWin->setFocus();
       return;
    }
    pull_infile=ui->Edit_pullInfile->text().toLocal8Bit().data();
    pull_outfile=ui->Edit_pullOutfile->text().toLocal8Bit().data();
    pull_win=ui->Edit_pullWin->text().toLocal8Bit().data();
    pull_bits=ui->Edit_pullBits->text().toInt();
    pull_fps=ui->Edit_pullFps->text().toInt();
    pull_gop=ui->Edit_pullGop->text().toInt();
    if(ui->Button_pullDISP->isChecked())
        pull_disp=1;
    else
        pull_disp=0;
    puller.init(const_cast<char*>(pull_infile.c_str()),const_cast<char*>(pull_outfile.c_str()),
                pull_fps,pull_bits,pull_gop,pull_disp,const_cast<char*>(pull_win.c_str()));
    puller.setId(2);
    puller.start();
    ui->Button_pullStart->setEnabled(false);
    ui->Button_pullStop->setEnabled(true);
    runFlag=true;

    infoStr="2pull start!";
    doDispInfo(const_cast<char*>(infoStr.c_str()));
    timerInfo->start(100);
}
void MainWindow::doPullStop(){
    puller.doStop();
    ui->Button_pullStart->setEnabled(true);
    ui->Button_pullStop->setEnabled(false);
    runFlag=false;

    timerInfo->stop();
    infoStr="2pull stop!";
    doDispInfo(const_cast<char*>(infoStr.c_str()));
}

void MainWindow::doTimeUpdate(){
    ui->timelabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    ui->datelabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
}
void MainWindow::selectImagePage(){
    if(runFlag&&ui->stackedWidget->currentIndex()!=1){
        QMessageBox::warning(this,"Warning","Something is running");
        return;
    }
    ui->stackedWidget->setCurrentIndex(1);
}
void MainWindow::selectVideoPage(){
    if(runFlag&&ui->stackedWidget->currentIndex()!=0){
        QMessageBox::warning(this,"Warning","Something is running");
        return;
    }
    ui->stackedWidget->setCurrentIndex(0);
}
void MainWindow::doDispInfo(char *info){
    if(info==NULL)
        return;
//    qDebug("%s",info);
    if(info[0]=='1'){
        char *tmp=&info[1];
//        qDebug("%s",tmp);
        ui->Text_push->append(QString::fromUtf8(tmp));
        ui->Text_push->moveCursor(QTextCursor::End);
    }else if(info[0]=='2'){
        char *tmp=&info[1];
//        qDebug("%s",tmp);
        ui->Text_pull->append(QString::fromUtf8(tmp));
        ui->Text_pull->moveCursor(QTextCursor::End);
    }else if(info[0]=='3'){
        char *tmp=&info[1];
//        qDebug("%s",tmp);
        ui->Text_imageRes->append(QString::fromUtf8(tmp));
        ui->Text_imageRes->moveCursor(QTextCursor::End);
    }else{
        return;
    }
}
void MainWindow::doDispInfoWrite(char *info){
    infoStr1=info;
}
void MainWindow::doDispInfoUpdate(){
    if(imagepusher.isFinished()&&timerImageInfo->isActive())
        timerImageInfo->stop();
    if(infoStr1!=infoStrBak)
    doDispInfo(const_cast<char*>(infoStr1.c_str()));
    infoStrBak=infoStr1;
}

void MainWindow::doOpenImage(){
//    qDebug("open");
    QString infile=QFileDialog::getOpenFileName(NULL,"Open File",QDir::homePath(),"Images (*.jpg)");
    if(infile.isEmpty()){
        QMessageBox::warning(this,"no file selected","Please select a valid file");
        return ;
    }
    ui->Edit_imagePath->setText(infile);
    ui->Label_image->setPixmap(QPixmap(infile).scaledToHeight(ui->Label_image->height()));
}
void MainWindow::doPushImage(){
    if(ui->Edit_imagePath->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_imagePath->setFocus();
       return;
    }
    if(ui->Edit_URL->text().isEmpty()){
       QMessageBox::warning(this,"Warning","content could not be empty");
       ui->Edit_URL->setFocus();
       return;
    }
    url=ui->Edit_URL->text().toLocal8Bit().data();
    filename=ui->Edit_imagePath->text().toLocal8Bit().data();
    imagepusher.init(url,filename,timeout);
    imagepusher.start();
    timerImageInfo->start(500);
}
