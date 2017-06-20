#ifndef STREAMDEALTHREAD_H
#define STREAMDEALTHREAD_H
#include <QThread>
#include "StreamDeal.h"

class StreamDealThread : public QThread
{
    Q_OBJECT
public:
    StreamDealThread();
    void run();
    void init(char *infile, char *outfile, int fps, int bits, int gop,
              int dispEnable, char *_winname);
    void doStop();
    void setId(int id);

private:
    StreamDeal stream;
};

#endif // STREAMDEALTHREAD_H
