#include "streamdealthread.h"
#include <QDebug>
StreamDealThread::StreamDealThread(){

}
void StreamDealThread::run(){
    stream.doInitWork();
    stream.doWork();
}
void StreamDealThread::init(char *infile, char *outfile, int fps, int bits, int gop, int dispEnable, char *_winname){
   stream.init(infile,outfile,fps,bits,gop,dispEnable,_winname);
}
void StreamDealThread::doStop(){
    stream.doStop();
    while(this->isRunning()){
        usleep(1000);
    }
    stream.doRealse();
}
void StreamDealThread::setId(int _id){
    stream.setId(_id);
}
