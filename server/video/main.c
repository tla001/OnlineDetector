#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "parser.h"
#include "utils.h"
#include "cuda.h"
#include "blas.h"
#include "connected_layer.h"
#include "image.h"

#include "libavutil/time.h"
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
//#include <libavutil/old_pix_fmts.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>


extern void predict_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int top);
extern void test_detector(char *datacfg, char *cfgfile, char *weightfile, char *filename, float thresh, float hier_thresh, char *outfile, int fullscreen);
extern void run_voxel(int argc, char **argv);
extern void run_yolo(int argc, char **argv);
extern void run_detector(int argc, char **argv);
extern void run_coco(int argc, char **argv);
extern void run_writing(int argc, char **argv);
extern void run_captcha(int argc, char **argv);
extern void run_nightmare(int argc, char **argv);
extern void run_dice(int argc, char **argv);
extern void run_compare(int argc, char **argv);
extern void run_classifier(int argc, char **argv);
extern void run_regressor(int argc, char **argv);
extern void run_char_rnn(int argc, char **argv);
extern void run_vid_rnn(int argc, char **argv);
extern void run_tag(int argc, char **argv);
extern void run_cifar(int argc, char **argv);
extern void run_go(int argc, char **argv);
extern void run_art(int argc, char **argv);
extern void run_super(int argc, char **argv);
extern void run_lsd(int argc, char **argv);

void average(int argc, char *argv[])
{
	char *cfgfile = argv[2];
	char *outfile = argv[3];
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	network sum = parse_network_cfg(cfgfile);

	char *weightfile = argv[4];   
	load_weights(&sum, weightfile);

	int i, j;
	int n = argc - 5;
	for(i = 0; i < n; ++i){
		weightfile = argv[i+5];   
		load_weights(&net, weightfile);
		for(j = 0; j < net.n; ++j){
			layer l = net.layers[j];
			layer out = sum.layers[j];
			if(l.type == CONVOLUTIONAL){
				int num = l.n*l.c*l.size*l.size;
				axpy_cpu(l.n, 1, l.biases, 1, out.biases, 1);
				axpy_cpu(num, 1, l.weights, 1, out.weights, 1);
				if(l.batch_normalize){
					axpy_cpu(l.n, 1, l.scales, 1, out.scales, 1);
					axpy_cpu(l.n, 1, l.rolling_mean, 1, out.rolling_mean, 1);
					axpy_cpu(l.n, 1, l.rolling_variance, 1, out.rolling_variance, 1);
				}
			}
			if(l.type == CONNECTED){
				axpy_cpu(l.outputs, 1, l.biases, 1, out.biases, 1);
				axpy_cpu(l.outputs*l.inputs, 1, l.weights, 1, out.weights, 1);
			}
		}
	}
	n = n+1;
	for(j = 0; j < net.n; ++j){
		layer l = sum.layers[j];
		if(l.type == CONVOLUTIONAL){
			int num = l.n*l.c*l.size*l.size;
			scal_cpu(l.n, 1./n, l.biases, 1);
			scal_cpu(num, 1./n, l.weights, 1);
			if(l.batch_normalize){
				scal_cpu(l.n, 1./n, l.scales, 1);
				scal_cpu(l.n, 1./n, l.rolling_mean, 1);
				scal_cpu(l.n, 1./n, l.rolling_variance, 1);
			}
		}
		if(l.type == CONNECTED){
			scal_cpu(l.outputs, 1./n, l.biases, 1);
			scal_cpu(l.outputs*l.inputs, 1./n, l.weights, 1);
		}
	}
	save_weights(sum, outfile);
}

void speed(char *cfgfile, int tics)
{
	if (tics == 0) tics = 1000;
	network net = parse_network_cfg(cfgfile);
	set_batch_network(&net, 1);
	int i;
	time_t start = time(0);
	image im = make_image(net.w, net.h, net.c*net.batch);
	for(i = 0; i < tics; ++i){
		network_predict(net, im.data);
	}
	double t = difftime(time(0), start);
	printf("\n%d evals, %f Seconds\n", tics, t);
	printf("Speed: %f sec/eval\n", t/tics);
	printf("Speed: %f Hz\n", tics/t);
}

void operations(char *cfgfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	int i;
	long ops = 0;
	for(i = 0; i < net.n; ++i){
		layer l = net.layers[i];
		if(l.type == CONVOLUTIONAL){
			ops += 2l * l.n * l.size*l.size*l.c * l.out_h*l.out_w;
		} else if(l.type == CONNECTED){
			ops += 2l * l.inputs * l.outputs;
		}
	}
	printf("Floating Point Operations: %ld\n", ops);
	printf("Floating Point Operations: %.2f Bn\n", (float)ops/1000000000.);
}

void oneoff(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	int oldn = net.layers[net.n - 2].n;
	int c = net.layers[net.n - 2].c;
	scal_cpu(oldn*c, .1, net.layers[net.n - 2].weights, 1);
	scal_cpu(oldn, 0, net.layers[net.n - 2].biases, 1);
	net.layers[net.n - 2].n = 9418;
	net.layers[net.n - 2].biases += 5;
	net.layers[net.n - 2].weights += 5*c;
	if(weightfile){
		load_weights(&net, weightfile);
	}
	net.layers[net.n - 2].biases -= 5;
	net.layers[net.n - 2].weights -= 5*c;
	net.layers[net.n - 2].n = oldn;
	printf("%d\n", oldn);
	layer l = net.layers[net.n - 2];
	copy_cpu(l.n/3, l.biases, 1, l.biases +   l.n/3, 1);
	copy_cpu(l.n/3, l.biases, 1, l.biases + 2*l.n/3, 1);
	copy_cpu(l.n/3*l.c, l.weights, 1, l.weights +   l.n/3*l.c, 1);
	copy_cpu(l.n/3*l.c, l.weights, 1, l.weights + 2*l.n/3*l.c, 1);
	*net.seen = 0;
	save_weights(net, outfile);
}

void oneoff2(char *cfgfile, char *weightfile, char *outfile, int l)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights_upto(&net, weightfile, 0, net.n);
		load_weights_upto(&net, weightfile, l, net.n);
	}
	*net.seen = 0;
	save_weights_upto(net, outfile, net.n);
}

void partial(char *cfgfile, char *weightfile, char *outfile, int max)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights_upto(&net, weightfile, 0, max);
	}
	*net.seen = 0;
	save_weights_upto(net, outfile, max);
}

#include "convolutional_layer.h"
void rescale_net(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	int i;
	for(i = 0; i < net.n; ++i){
		layer l = net.layers[i];
		if(l.type == CONVOLUTIONAL){
			rescale_weights(l, 2, -.5);
			break;
		}
	}
	save_weights(net, outfile);
}

void rgbgr_net(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	int i;
	for(i = 0; i < net.n; ++i){
		layer l = net.layers[i];
		if(l.type == CONVOLUTIONAL){
			rgbgr_weights(l);
			break;
		}
	}
	save_weights(net, outfile);
}

void reset_normalize_net(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if (weightfile) {
		load_weights(&net, weightfile);
	}
	int i;
	for (i = 0; i < net.n; ++i) {
		layer l = net.layers[i];
		if (l.type == CONVOLUTIONAL && l.batch_normalize) {
			denormalize_convolutional_layer(l);
		}
		if (l.type == CONNECTED && l.batch_normalize) {
			denormalize_connected_layer(l);
		}
		if (l.type == GRU && l.batch_normalize) {
			denormalize_connected_layer(*l.input_z_layer);
			denormalize_connected_layer(*l.input_r_layer);
			denormalize_connected_layer(*l.input_h_layer);
			denormalize_connected_layer(*l.state_z_layer);
			denormalize_connected_layer(*l.state_r_layer);
			denormalize_connected_layer(*l.state_h_layer);
		}
	}
	save_weights(net, outfile);
}

layer normalize_layer(layer l, int n)
{
	int j;
	l.batch_normalize=1;
	l.scales = calloc(n, sizeof(float));
	for(j = 0; j < n; ++j){
		l.scales[j] = 1;
	}
	l.rolling_mean = calloc(n, sizeof(float));
	l.rolling_variance = calloc(n, sizeof(float));
	return l;
}

void normalize_net(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	int i;
	for(i = 0; i < net.n; ++i){
		layer l = net.layers[i];
		if(l.type == CONVOLUTIONAL && !l.batch_normalize){
			net.layers[i] = normalize_layer(l, l.n);
		}
		if (l.type == CONNECTED && !l.batch_normalize) {
			net.layers[i] = normalize_layer(l, l.outputs);
		}
		if (l.type == GRU && l.batch_normalize) {
			*l.input_z_layer = normalize_layer(*l.input_z_layer, l.input_z_layer->outputs);
			*l.input_r_layer = normalize_layer(*l.input_r_layer, l.input_r_layer->outputs);
			*l.input_h_layer = normalize_layer(*l.input_h_layer, l.input_h_layer->outputs);
			*l.state_z_layer = normalize_layer(*l.state_z_layer, l.state_z_layer->outputs);
			*l.state_r_layer = normalize_layer(*l.state_r_layer, l.state_r_layer->outputs);
			*l.state_h_layer = normalize_layer(*l.state_h_layer, l.state_h_layer->outputs);
			net.layers[i].batch_normalize=1;
		}
	}
	save_weights(net, outfile);
}

void statistics_net(char *cfgfile, char *weightfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if (weightfile) {
		load_weights(&net, weightfile);
	}
	int i;
	for (i = 0; i < net.n; ++i) {
		layer l = net.layers[i];
		if (l.type == CONNECTED && l.batch_normalize) {
			printf("Connected Layer %d\n", i);
			statistics_connected_layer(l);
		}
		if (l.type == GRU && l.batch_normalize) {
			printf("GRU Layer %d\n", i);
			printf("Input Z\n");
			statistics_connected_layer(*l.input_z_layer);
			printf("Input R\n");
			statistics_connected_layer(*l.input_r_layer);
			printf("Input H\n");
			statistics_connected_layer(*l.input_h_layer);
			printf("State Z\n");
			statistics_connected_layer(*l.state_z_layer);
			printf("State R\n");
			statistics_connected_layer(*l.state_r_layer);
			printf("State H\n");
			statistics_connected_layer(*l.state_h_layer);
		}
		printf("\n");
	}
}

void denormalize_net(char *cfgfile, char *weightfile, char *outfile)
{
	gpu_index = -1;
	network net = parse_network_cfg(cfgfile);
	if (weightfile) {
		load_weights(&net, weightfile);
	}
	int i;
	for (i = 0; i < net.n; ++i) {
		layer l = net.layers[i];
		if (l.type == CONVOLUTIONAL && l.batch_normalize) {
			denormalize_convolutional_layer(l);
			net.layers[i].batch_normalize=0;
		}
		if (l.type == CONNECTED && l.batch_normalize) {
			denormalize_connected_layer(l);
			net.layers[i].batch_normalize=0;
		}
		if (l.type == GRU && l.batch_normalize) {
			denormalize_connected_layer(*l.input_z_layer);
			denormalize_connected_layer(*l.input_r_layer);
			denormalize_connected_layer(*l.input_h_layer);
			denormalize_connected_layer(*l.state_z_layer);
			denormalize_connected_layer(*l.state_r_layer);
			denormalize_connected_layer(*l.state_h_layer);
			l.input_z_layer->batch_normalize = 0;
			l.input_r_layer->batch_normalize = 0;
			l.input_h_layer->batch_normalize = 0;
			l.state_z_layer->batch_normalize = 0;
			l.state_r_layer->batch_normalize = 0;
			l.state_h_layer->batch_normalize = 0;
			net.layers[i].batch_normalize=0;
		}
	}
	save_weights(net, outfile);
}

void mkimg(char *cfgfile, char *weightfile, int h, int w, int num, char *prefix)
{
	network net = load_network(cfgfile, weightfile, 0);
	image *ims = get_weights(net.layers[0]);
	int n = net.layers[0].n;
	int z;
	for(z = 0; z < num; ++z){
		image im = make_image(h, w, 3);
		fill_image(im, .5);
		int i;
		for(i = 0; i < 100; ++i){
			image r = copy_image(ims[rand()%n]);
			rotate_image_cw(r, rand()%4);
			random_distort_image(r, 1, 1.5, 1.5);
			int dx = rand()%(w-r.w);
			int dy = rand()%(h-r.h);
			ghost_image(r, im, dx, dy);
			free_image(r);
		}
		char buff[256];
		sprintf(buff, "%s/gen_%d", prefix, z);
		save_image(im, buff);
		free_image(im);
	}
}

void visualize(char *cfgfile, char *weightfile)
{
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	visualize_network(net);
#ifdef OPENCV
	cvWaitKey(0);
#endif
}

int exitFlag=0;
void sigHandle(int signal){
	if(signal==SIGINT){
		printf("rec SIGINT\n");
		exitFlag=1;
	}
}
//×××××××tla001*******/
double get_wall_time1()
{
	struct timeval time;
	if (gettimeofday(&time,NULL)){
		return 0;
	}
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
IplImage *src;
int ready=0;
void recStream(void *arg){
	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameRGB;
	uint8_t *out_buffer;
	AVPacket *packet;
	//int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	//输入文件路径
	//	char filepath[] = "rtmp://219.216.87.170/vod/test.flv";
	char filepath[] = "rtmp://219.216.87.170/live/test1";
	int frame_cnt;

	printf("wait for playing %s\n", filepath);
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	printf("size %ld\tduration %ld\n", pFormatCtx->probesize,
			pFormatCtx->max_analyze_duration);
	pFormatCtx->probesize = 20000000;
	pFormatCtx->max_analyze_duration = 2000;
	//	pFormatCtx->interrupt_callback.callback = timout_callback;
	//	pFormatCtx->interrupt_callback.opaque = pFormatCtx;
	//	pFormatCtx->flags |= AVFMT_FLAG_NONBLOCK;

	AVDictionary* opt = NULL;
	av_dict_set(&opt, "fflags", "nobuffer", 0);
	//av_dict_set(&opt, "max_delay", "100000", 0);
	//	av_dict_set(&options, "rtmp_transport", "tcp", 0);
	//av_dict_set(&opt, "stimeout", "6", 0);

	printf("wating for opening file\n");
	if (avformat_open_input(&pFormatCtx, filepath, NULL, &opt) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	av_dict_free(&opt);
	printf("wating for finding stream\n");
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}
	/*
	 * 在此处添加输出视频信息的代码
	 * 取自于pFormatCtx，使用fprintf()
	 */
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();
	out_buffer = (uint8_t *) av_malloc(
			avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
				pCodecCtx->height));
	avpicture_fill((AVPicture *) pFrameRGB, out_buffer, AV_PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket *) av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
			AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
	CvSize imagesize;
	imagesize.width = pCodecCtx->width;
	imagesize.height = pCodecCtx->height;
	src = cvCreateImageHeader(imagesize, IPL_DEPTH_8U, 3);
	cvSetData(src, out_buffer, imagesize.width * 3);



	while (av_read_frame(pFormatCtx, packet) >= 0/*&&!exitFlag*/) {
		if (packet->stream_index == videoindex) {
			/*
			 * 在此处添加输出H264码流的代码
			 * 取自于packet，使用fwrite()
			 */
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
					packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {

				sws_scale(img_convert_ctx,
						(const uint8_t* const *) pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pFrameRGB->data,
						pFrameRGB->linesize);

				// cvShowImage("src",src);
				//cvWaitKey(25);
				// printf("Decoded frame index: %d\n", frame_cnt);

				/*
				 * 在此处添加输出YUV的代码
				 * 取自于pFrameYUV，使用fwrite()
				 */

				// frame_cnt++;

				ready=1;
			}
		}
		av_free_packet(packet);
	}
	//free_image(im);



	sws_freeContext(img_convert_ctx);

	av_frame_free(&pFrameRGB);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

}
IplImage *disp;
int sendready=0;
void pushStream(){
	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ofmt_ctx = NULL;
	AVCodecContext *ofcodec_ctx;
	AVCodec *ocodec;
	AVStream *out_stream;
	AVFrame *pFrame, *pFrameYUV420;
	struct SwsContext *out_conCtx;
	unsigned char *out_buffer;
	AVPacket  outpkg;
	const char  *out_filename;
	int ret, i;
	int got_picture;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;
	//  in_filename = "test.mp4";
	//  in_filename = "/dev/video0";
	out_filename = "rtmp://219.216.87.170/live/test2";
	// out_filename = "a.flv";

	av_register_all();
	avdevice_register_all();
	avformat_network_init();

	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);
	if (!ofmt_ctx) {
		printf("could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	out_stream = avformat_new_stream(ofmt_ctx, NULL);
	if (!out_stream) {
		printf("failed allocating output stream\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	while(!sendready)
		usleep(100);
	ofcodec_ctx = out_stream->codec;
	ofcodec_ctx->codec_id = AV_CODEC_ID_H264;
	ofcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	ofcodec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	ofcodec_ctx->width = disp->width;
	ofcodec_ctx->height = disp->height;
	ofcodec_ctx->time_base.den = 30;
	ofcodec_ctx->time_base.num = 1;
	printf("timebase %d %d\n", ofcodec_ctx->time_base.den,
			ofcodec_ctx->time_base.num);
	ofcodec_ctx->bit_rate = 700000;
	ofcodec_ctx->gop_size = 5;
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
		goto end;
	}
	if (avcodec_open2(ofcodec_ctx, ocodec, NULL) < 0) {
		printf("open encoder err\n");
		goto end;
	}

	printf("**************** output file info ******************\n");
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("could not open output url '%s'\n", out_filename);
			goto end;
		}
	}

	/*******************************************/
	pFrame = av_frame_alloc();
	pFrameYUV420 = av_frame_alloc();
    printf("disp width %d height %d\n", disp->width, disp->height);
	avpicture_fill((AVPicture*) pFrame, disp->imageData, AV_PIX_FMT_BGR24,
			disp->width, disp->height);
	//  printf("fmt %d\twidth %d\theight %d\n", pFrameBGR->format, pFrameBGR->width,
	//          pFrameBGR->height);
	out_buffer = (unsigned char *) av_malloc(
			avpicture_get_size(AV_PIX_FMT_YUV420P, ofcodec_ctx->width,
				ofcodec_ctx->height));
	avpicture_fill((AVPicture*) pFrameYUV420, out_buffer, AV_PIX_FMT_YUV420P,
			ofcodec_ctx->width, ofcodec_ctx->height);

	//  printf("fmt %d\twidth %d\theight %d\n", pFrameYUV420->format,
	//          pFrameYUV420->width, pFrameYUV420->height);

	out_conCtx = sws_getContext(ofcodec_ctx->width, ofcodec_ctx->height,
			AV_PIX_FMT_BGR24, ofcodec_ctx->width, ofcodec_ctx->height,
			ofcodec_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);


	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("error occurred when opening output URL\n");
		goto end;
	}
	start_time = av_gettime();
	pFrameYUV420->format = AV_PIX_FMT_YUV420P;
	pFrameYUV420->width = ofcodec_ctx->width;
	pFrameYUV420->height = ofcodec_ctx->height;
	av_new_packet(&outpkg, ofcodec_ctx->width * ofcodec_ctx->height * 3);
	videoindex=0;
	while (1) {
		if(sendready){
			pFrame->pts = frame_index;


			cvShowImage("sendback", disp);
			// cvWaitKey(1);
			frame_index++;
			sws_scale(out_conCtx,
					(const unsigned char * const *) pFrame->data,
					pFrame->linesize, 0, ofcodec_ctx->height,
					pFrameYUV420->data, pFrameYUV420->linesize);
			//              printf("yuv420 fmt %d\twidth %d\theight %d\n",
			//                      pFrameYUV420->format, pFrameYUV420->width,
			//                      pFrameYUV420->height);

			got_picture = 0;
			pFrameYUV420->pts = pFrame->pts;
			ret = avcodec_encode_video2(ofcodec_ctx, &outpkg, pFrameYUV420,
					&got_picture);
			if (ret < 0) {
				printf("encode err\n");
				goto end;
			}
			if (got_picture == 1) {
				printf("encode  frame %d\n", frame_index);
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
				//                  av_write_frame(ofmt_ctx, &outpkg);
				av_interleaved_write_frame(ofmt_ctx, &outpkg);
				av_free_packet(&outpkg);
			}
            sendready=0;

		}
        else{
            usleep(1000);
        }
	}


	av_write_trailer(ofmt_ctx);
end: 
	sws_freeContext(out_conCtx);
	free(out_buffer);
	av_free(pFrameYUV420);
	avcodec_close(ofcodec_ctx);
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		exit(-1);
	}
	exit(0);
}
void image_to_ipl(image p,IplImage *disp){
	int x,y,k;
	constrain_image(p);
	rgbgr_image(p);
	int step = disp->widthStep;
	for(y = 0; y < p.h; ++y){
		for(x = 0; x < p.w; ++x){
			for(k= 0; k < p.c; ++k){
				disp->imageData[y*step + x*p.c + k] = (unsigned char)(get_pixel(p,x,y,k)*255);
			}
		}
	}

}
int main(int argc, char **argv)
{

	pthread_t tid1;
	pthread_create(&tid1,NULL,recStream,NULL);
    pthread_t tid2;
    pthread_create(&tid2,NULL,pushStream,NULL);
	gpu_index = find_int_arg(argc, argv, "-i", 0);
	if(find_arg(argc, argv, "-nogpu")) {
		gpu_index = -1;
	}

#ifndef GPU
	gpu_index = -1;
#else
	if(gpu_index >= 0){
		cuda_set_device(gpu_index);
	}
#endif

	float thresh = find_float_arg(argc, argv, "-thresh", .24);
	// char *filename ="test.jpg";
	// char *outfile = find_char_arg(argc, argv, "-out", 0);
	// int fullscreen = find_arg(argc, argv, "-fullscreen");
	char *cfgfile="cfg/yolo.cfg";
	char *weightfile="yolo.weights";
	char *datacfg="cfg/coco.data";
	float hier_thresh=0.5;
	list *options = read_data_cfg(datacfg);
	char *name_list = option_find_str(options, "names", "data/names.list");
	char **names = get_labels(name_list);

	image **alphabet = load_alphabet();
	network net = parse_network_cfg(cfgfile);
	if(weightfile){
		load_weights(&net, weightfile);
	}
	set_batch_network(&net, 1);
	srand(2222222);
	clock_t time;
	char buff[256];
	char *input = buff;
	int j;
	float nms=.4;

	//if(signal(SIGINT,sigHandle)==SIG_ERR){
	//	perror("set signal err");
	//}



	layer l = net.layers[net.n-1];

	box *boxes = calloc(l.w*l.h*l.n, sizeof(box));
	float **probs = calloc(l.w*l.h*l.n, sizeof(float *));
	for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = calloc(l.classes + 1, sizeof(float *));

	cvNamedWindow("yolo detector", CV_WINDOW_AUTOSIZE);

	//image im=make_image(pCodecCtx->width,pCodecCtx->height,3);

	int frame_cnt = 0;
	int num = 0;
	double demo_time;
	float fps = 0;
	while(!ready)
		usleep(10);
	image im=make_image(src->width,src->height,3);
	disp = cvCreateImage(cvSize(src->width,src->height), IPL_DEPTH_8U, 3);
	while(1){
		if(ready){
			fps = 1./(get_wall_time1() - demo_time);
			demo_time = get_wall_time1();
			//cvShowImage("src",src);
			ipl_into_image(src,im);
			// image im= load_image_color("test.jpg",0,0);
			image sized = letterbox_image(im, net.w, net.h);
			float *X = sized.data;
			time=clock();
			network_predict(net, X);
			//printf("be thresh=%f  hier_thresh=%f\n", thresh,hier_thresh);
			get_region_boxes(l, im.w, im.h, net.w, net.h, thresh, probs, boxes, 0, 0, hier_thresh, 1);
			if (nms) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
			//else if (nms) do_nms_sort(boxes, probs, l.w*l.h*l.n, l.classes, nms);
			printf("\033[2J");
			printf("\033[1;1H");
			printf("\nFPS:%.1f\n",fps);
			printf("current frame index: %ld\n",frame_cnt++);
			printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
			printf("Objects:\n\n");
			printf("*********************************************\n");
			//printf("thresh=%f  hier_thresh=%f\n", thresh,hier_thresh);
			draw_detections1(im, l.w*l.h*l.n, thresh, boxes, probs, names, alphabet, l.classes);

			image_to_ipl(im,disp);
            sendready=1;
			cvShowImage("yolo detector",disp);
			//show_image(im, "yolo detector");
			cvWaitKey(1);


			free_image(sized);
			ready=0;
		}
		else{
			//printf("main thread is waiting\n");
			usleep(1000);
		}

	}

	free_image(im);
	cvReleaseImage(&disp);
	cvDestroyAllWindows();

	free(boxes);
	free_ptrs((void **)probs, l.w*l.h*l.n);
	return 0;
}

