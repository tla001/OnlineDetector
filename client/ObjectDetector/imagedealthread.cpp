#include "imagedealthread.h"

ImageDealThread::ImageDealThread(){

}
void ImageDealThread::init(string url, string fileName, int timeout){
    image.init(url,fileName,timeout);
}
void ImageDealThread::run(){
    image.doWork();
}
