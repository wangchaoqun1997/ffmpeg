#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include "RGBA_YUV.cpp"
}

static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt,
	FILE* outfile)
{
	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %3d \n", frame->pts);

	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}	
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}

		printf("Write packet %3d  (size=%5d)\n", pkt->pts, pkt->size);

		av_grow_packet(pkt,16);
		pkt->data[pkt->size - 16] = 0x16;
		pkt->data[pkt->size - 15] = 0x15;
		pkt->data[pkt->size - 14] = 0x14;
		pkt->data[pkt->size - 13] = 0x13;
		pkt->data[pkt->size - 12] = 0x12;
		pkt->data[pkt->size - 11] = 0x11;
		pkt->data[pkt->size - 10] = 0x10;
		pkt->data[pkt->size - 9] = 0x9;
		pkt->data[pkt->size - 8] = 0x8;
		pkt->data[pkt->size - 7] = 0x7;
		pkt->data[pkt->size - 6] = 0x6;
		pkt->data[pkt->size - 5] = 0x5;
		pkt->data[pkt->size-4] = 0x4;
		pkt->data[pkt->size - 3] = 0x3;
		pkt->data[pkt->size - 2] = 0x2;
		pkt->data[pkt->size -1] = 0x1;
		printf("====Write packet %3d  (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
}

int main(int argc, char** argv)
{
	const char* filename, * codec_name;
	const AVCodec* codec;
	AVCodecContext* c = NULL;
	int i, ret, x, y;
	FILE* f,*rf;
	AVFrame* frame;
	AVPacket* pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	//if (argc <= 2) {
	//	fprintf(stderr, "Usage: %s <output file> <codec name>\n", argv[0]);
	//	//exit(0);
	//}
	filename = "rgbaFile_640_480.h264";
	const char *readfilename = "rgbaFile_640_480_50.yuv";
	codec_name ="AV_CODEC_ID_H264";


	simplest_rgb24_to_yuv420("rgbaFile_640_480_50.rgb",640,480,50, filename);
	//return 0;

	/* find the mpeg1video encoder */
	//codec = avcodec_find_encoder_by_name(codec_name);
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec '%s' not found\n", codec_name);
		exit(1);
	}
	
	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);
	
	AVRational a;
	a.num = 1;
	a.den = 25;
	AVRational b;
	b.num = 25;
	b.den = 1;

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = 640;
	c->height = 480;
	/* frames per second */
	c->time_base = a;
	c->framerate = b;

	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		//fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}
	
	f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}
	rf = fopen(readfilename, "rb");
	if (!rf) {
		fprintf(stderr, "Could not open %s\n", readfilename);
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	fprintf(stderr, "av_frame_get_buffer %d  %d  %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2]);
	ret = av_frame_get_buffer(frame,0);
	fprintf(stderr, "==av_frame_get_buffer %d  %d  %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2]);

	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	/* encode 1 second of video */
	for (i = 0; i <50; i++) {
		fflush(stdout);

		/* make sure the frame data is writable */
		ret = av_frame_make_writable(frame);
		if (ret < 0)
			exit(1);


		fread(frame->data[0], c->height * c->width, 1, rf);
		fread(frame->data[1], c->height * c->width/4, 1, rf);
		fread(frame->data[2], c->height * c->width / 4, 1, rf);

		///* prepare a dummy image */
		///* Y */
		//for (y = 0; y < c->height; y++) {
		//	for (x = 0; x < c->width; x++) {
		//		frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
		//	}
		//}

		///* Cb and Cr */
		//for (y = 0; y < c->height / 2; y++) {
		//	for (x = 0; x < c->width / 2; x++) {
		//		frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
		//		frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
		//	}
		//}

		frame->pts = i;

		/* encode the image */
		encode(c, frame, pkt, f);
	}
	
	/* flush the encoder */
	encode(c, NULL, pkt, f);

	/* add sequence end code to have a real MPEG file */
	if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
		fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	
	return 0;
}
