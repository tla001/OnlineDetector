#ifndef IMAGEDEALTHREAD_H
#define IMAGEDEALTHREAD_H
#include <QThread>
#include "imagedeal.h"

class ImageDealThread : public QThread
{
    Q_OBJECT
public:
    ImageDealThread();

    void init(string url,string fileName,int timeout);
    void run();

private:
    ImageDeal image;
};

#endif // IMAGEDEALTHREAD_H
