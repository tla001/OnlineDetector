#ifndef IMAGEDEAL_H
#define IMAGEDEAL_H

#include "Include.h"

class ImageDeal
{
public:
    ImageDeal();
    virtual ~ImageDeal();

    void init(string _url,string _fileName,int _timeout);
    void doWork();


private:
    CURL *pCurl;
    CURLcode code;
    string url;
    string fileName;
    int timeout;

    char buff[1000];

};

#endif // IMAGEDEAL_H
