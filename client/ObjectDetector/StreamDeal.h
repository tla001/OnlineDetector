/*
 * StreamDeal.h
 *
 *  Created on: Jun 19, 2017
 *      Author: tla001
 */

#ifndef STREAMDEAL_H_
#define STREAMDEAL_H_

#include "Include.h"


class StreamDeal {
public:
	StreamDeal();
	virtual ~StreamDeal();

	void init(char *infile, char *outfile, int fps, int bits, int gop,
			int dispEnable, char *_winname);
    void setId(int id);
    void doInitWork();
	void doWork();
	void doRealse();
	void doStop();

private:
	AVOutputFormat *ofmt;
	AVFormatContext *ifmt_ctx, *ofmt_ctx;
	AVCodecContext *ifcodec_ctx, *ofcodec_ctx;
	AVCodec *icodec, *ocodec;
	AVStream *out_stream;
	AVFrame *pFrame, *pFrameYUV420, *pFrameBGR;
	struct SwsContext *in_conCtx, *out_conCtx;
	unsigned char *in_buffer, *out_buffer;
	AVPacket inpkg, outpkg;
	char *in_filename, *out_filename;
	int ret;
	int got_picture;
	IplImage *image;
	int videoindex;
	int frame_index;
	int errFlag;
	int stopFlag;
	int dispFlag;
	char *winname;
    int fps;
    int bits;
    int gop;
    int id;

public:
	static void test() {
		char *infile = "/dev/video0";
		char *outfile = "test.flv";
		int fps = 30;
		int gop = 5;
		int bits = 1000000;
		int disp = 1;
		char *winname = "demo";
		StreamDeal streamer;
		streamer.init(infile, outfile, fps, bits, gop, disp, winname);
        streamer.doInitWork();
		streamer.doWork();

}
};

#endif /* STREAMDEAL_H_ */
