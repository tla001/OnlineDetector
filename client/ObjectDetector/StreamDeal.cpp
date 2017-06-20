/*
 * StreamDeal.cpp
 *
 *  Created on: Jun 19, 2017
 *      Author: tla001
 */

#include "StreamDeal.h"
#include <QDebug>
#include "mainwindow.h"
extern MainWindow *w;

StreamDeal::StreamDeal() {
	ofmt = NULL;
	ifmt_ctx = NULL;
	ofmt_ctx = NULL;
	ifcodec_ctx = NULL;
	ofcodec_ctx = NULL;
	icodec = NULL;
	ocodec = NULL;
	out_stream = NULL;
	pFrame = NULL;
	pFrameYUV420 = NULL;
	pFrameBGR = NULL;
	in_conCtx = NULL;
	out_conCtx = NULL;
	in_buffer = NULL;
	out_buffer = NULL;
	in_filename = NULL;
	out_filename = NULL;
	ret = 0;
	got_picture = 0;
	image = NULL;
	videoindex = -1;
	frame_index = 0;
	errFlag = 0;
	stopFlag = 0;
	dispFlag = 0;
    fps=0;
    bits=0;
    gop=0;
	winname = NULL;
    id=-1;

	av_register_all();
	avdevice_register_all();
	avformat_network_init();

}

StreamDeal::~StreamDeal() {
	// TODO Auto-generated destructor stub
	if (errFlag && !stopFlag)
		doRealse();
}
void StreamDeal::init(char *infile, char *outfile, int _fps, int _bits, int _gop,
		int dispEnable, char *_winname) {
    id=-1;
    fps=_fps;
    bits=_bits;
    gop=_gop;
	winname = _winname;
	dispFlag = dispEnable;
	stopFlag = 0;
	errFlag = 0;
	videoindex = -1;
	frame_index = 0;
	ret = 0;
	in_filename = infile;
	out_filename = outfile;
	ifmt_ctx = avformat_alloc_context();
	ifmt_ctx->probesize = 20000000;
	ifmt_ctx->max_analyze_duration = 2000;


//     qDebug("init ok\n");
}
void StreamDeal::setId(int _id){
    id=_id;
}

void StreamDeal::doInitWork(){
    AVDictionary* options = NULL;
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "max_delay", "100000", 0);

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &options)) < 0) {
        printf("open input file err\n");
        errFlag = 1;
        exit(-1);
    }
    av_dict_free(&options);
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf("failed to retrieve input stream information\n");
        errFlag = 1;
        exit(-1);
    }
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    ifcodec_ctx = ifmt_ctx->streams[videoindex]->codec;
    icodec = avcodec_find_decoder(ifcodec_ctx->codec_id);
    if (icodec == NULL) {
        printf("icodec not find\n");
        errFlag = 1;
        exit(-1);
    }
    if (avcodec_open2(ifcodec_ctx, icodec, NULL) < 0) {
        printf("open icodec err\n");
        errFlag = 1;
        exit(-1);
    }
    printf("**************** input file info ******************\n");
    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);
    if (!ofmt_ctx) {
        printf("could not create output context\n");
        ret = AVERROR_UNKNOWN;
        errFlag = 1;
        exit(-1);
    }
    ofmt = ofmt_ctx->oformat;
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream) {
        printf("failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
        errFlag = 1;
        exit(-1);
    }
    ofcodec_ctx = out_stream->codec;
    ofcodec_ctx->codec_id = AV_CODEC_ID_H264;
    ofcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    ofcodec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ofcodec_ctx->width = ifcodec_ctx->width;
    ofcodec_ctx->height = ifcodec_ctx->height;
    ofcodec_ctx->time_base.den = fps;
    ofcodec_ctx->time_base.num = 1;
    printf("timebase %d %d\n", ofcodec_ctx->time_base.den,
            ofcodec_ctx->time_base.num);
    ofcodec_ctx->bit_rate = bits;
    ofcodec_ctx->gop_size = gop;
    ofcodec_ctx->me_range = 16;
    ofcodec_ctx->max_qdiff = 4;
    ofcodec_ctx->qmin = 10;
    ofcodec_ctx->qmax = 51;
    ofcodec_ctx->qcompress = 0.6;
    if (ofcodec_ctx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(ofcodec_ctx->priv_data, "preset", "slow", 0);
        ofcodec_ctx->max_b_frames = 1;
    }

    out_stream->codec->codec_tag = 0;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    ocodec = avcodec_find_encoder(ofcodec_ctx->codec_id);
    if (!ocodec) {
        printf("find encoder err\n");
        errFlag = 1;
        exit(-1);
    }
    if (avcodec_open2(ofcodec_ctx, ocodec, NULL) < 0) {
        printf("open encoder err\n");
        errFlag = 1;
        exit(-1);
    }

    printf("**************** output file info ******************\n");
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("could not open output url '%s'\n", out_filename);
            errFlag = 1;
            exit(-1);
        }
    }

    /*******************************************/
    pFrame = av_frame_alloc();
    pFrameYUV420 = av_frame_alloc();
    pFrameBGR = av_frame_alloc();
    in_buffer = (unsigned char *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_BGR24, ifcodec_ctx->width,
                    ifcodec_ctx->height));
    avpicture_fill((AVPicture*) pFrameBGR, in_buffer, AV_PIX_FMT_BGR24,
            ifcodec_ctx->width, ifcodec_ctx->height);
    //	printf("fmt %d\twidth %d\theight %d\n", pFrameBGR->format, pFrameBGR->width,
    //			pFrameBGR->height);
    out_buffer = (unsigned char *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, ofcodec_ctx->width,
                    ofcodec_ctx->height));
    avpicture_fill((AVPicture*) pFrameYUV420, out_buffer, AV_PIX_FMT_YUV420P,
            ofcodec_ctx->width, ofcodec_ctx->height);

    //	printf("fmt %d\twidth %d\theight %d\n", pFrameYUV420->format,
    //			pFrameYUV420->width, pFrameYUV420->height);

    in_conCtx = sws_getContext(ifcodec_ctx->width, ifcodec_ctx->height,
            ifcodec_ctx->pix_fmt, ifcodec_ctx->width, ifcodec_ctx->height,
            AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
    out_conCtx = sws_getContext(ifcodec_ctx->width, ifcodec_ctx->height,
            ifcodec_ctx->pix_fmt, ofcodec_ctx->width, ofcodec_ctx->height,
            ofcodec_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    image = cvCreateImageHeader(cvSize(ifcodec_ctx->width, ifcodec_ctx->height),
    IPL_DEPTH_8U, 3);
    cvSetData(image, in_buffer, ifcodec_ctx->width * 3);
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        printf("error occurred when opening output URL\n");
        errFlag = 1;
        exit(-1);
    }
    pFrameYUV420->format = AV_PIX_FMT_YUV420P;
    pFrameYUV420->width = ofcodec_ctx->width;
    pFrameYUV420->height = ofcodec_ctx->height;
    av_new_packet(&outpkg, ofcodec_ctx->width * ofcodec_ctx->height * 3);
    if (dispFlag) {
        cvNamedWindow(winname, CV_WINDOW_AUTOSIZE);
        cvMoveWindow(winname, 100, 100);
    }
}

void StreamDeal::doWork() {
//    qDebug("thread is up");
    char buff[100];
    struct timeval startTime,endTime;
    float timeuse=0.0;
	while (!stopFlag && av_read_frame(ifmt_ctx, &inpkg) >= 0) {
//        qDebug("get one frame");
        gettimeofday(&startTime,NULL);
		inpkg.dts = av_rescale_q_rnd(inpkg.dts,
				ifmt_ctx->streams[videoindex]->time_base,
				ifmt_ctx->streams[videoindex]->codec->time_base,
				(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		inpkg.pts = av_rescale_q_rnd(inpkg.pts,
				ifmt_ctx->streams[videoindex]->time_base,
				ifmt_ctx->streams[videoindex]->codec->time_base,
				(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		//decode
		if (inpkg.stream_index == videoindex) {
			ret = avcodec_decode_video2(ifcodec_ctx, pFrame, &got_picture,
					&inpkg);
			if (ret < 0) {
				printf("decode err\n");
				exit(-1);
			}
			if (got_picture) {
				pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

				sws_scale(in_conCtx,
						(const unsigned char * const *) pFrame->data,
						pFrame->linesize, 0, ifcodec_ctx->height,
						pFrameBGR->data, pFrameBGR->linesize);
//				printf("bgr fmt %d\twidth %d\theight %d\n", pFrameBGR->format,
//						pFrameBGR->width, pFrameBGR->height);
				if (dispFlag) {
//                    qDebug("fresh frame %d",frame_index);
					cvShowImage(winname, image);
                    usleep(1000);
//					cvWaitKey(1);
				}
				frame_index++;
				sws_scale(out_conCtx,
						(const unsigned char * const *) pFrame->data,
						pFrame->linesize, 0, ofcodec_ctx->height,
						pFrameYUV420->data, pFrameYUV420->linesize);
//				printf("yuv420 fmt %d\twidth %d\theight %d\n",
//						pFrameYUV420->format, pFrameYUV420->width,
//						pFrameYUV420->height);

				got_picture = 0;
				pFrameYUV420->pts = pFrame->pts;
				ret = avcodec_encode_video2(ofcodec_ctx, &outpkg, pFrameYUV420,
						&got_picture);
				if (ret < 0) {
					errFlag = 1;
					exit(-1);
				}
				if (got_picture == 1) {
//					printf("encode  frame %d\n", frame_index);

					outpkg.stream_index = out_stream->index;
					outpkg.dts = av_rescale_q_rnd(outpkg.dts,
							ofmt_ctx->streams[videoindex]->codec->time_base,
							ofmt_ctx->streams[videoindex]->time_base,
							(enum AVRounding) (AV_ROUND_NEAR_INF
									| AV_ROUND_PASS_MINMAX));
					outpkg.pts = av_rescale_q_rnd(outpkg.pts,
							ofmt_ctx->streams[videoindex]->codec->time_base,
							ofmt_ctx->streams[videoindex]->time_base,
							(enum AVRounding) (AV_ROUND_NEAR_INF
									| AV_ROUND_PASS_MINMAX));
					outpkg.duration = av_rescale_q(outpkg.duration,
							ofmt_ctx->streams[videoindex]->codec->time_base,
							ofmt_ctx->streams[videoindex]->time_base);
//					av_write_frame(ofmt_ctx, &outpkg);
					av_interleaved_write_frame(ofmt_ctx, &outpkg);
					av_free_packet(&outpkg);
				}
			}
		}
        gettimeofday(&endTime,NULL);
        timeuse=1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec);
        timeuse/=1000;
        sprintf(buff,"%ddeal %d frame with %.2f ms", id,frame_index-1,timeuse);
        w->doDispInfoWrite(buff);
		av_free_packet(&inpkg);
	}
	av_write_trailer(ofmt_ctx);
}
void StreamDeal::doRealse() {
	if (dispFlag) {
		cvDestroyWindow(winname);
	}
	sws_freeContext(in_conCtx);
	sws_freeContext(out_conCtx);
	free(in_buffer);
	free(out_buffer);
	av_free(pFrameYUV420);
	av_free(pFrameBGR);
	avcodec_close(ifcodec_ctx);
	avcodec_close(ofcodec_ctx);
	avformat_close_input(&ifmt_ctx);
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
}
void StreamDeal::doStop() {
	stopFlag = 1;
}
