// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

AVFormatContext* fmt_ctx = NULL;
AVCodecContext* dec_ctx = NULL;
const char* src_filename = "media1.mp4", * outfilename;
int video_stream_idx = -1;
AVFrame* frame = NULL;

AVPacket pkt;





void model_demuxing() {


	/* open input file, and allocate format context */
	if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", src_filename);
		exit(1);
	}

	/* retrieve stream information */
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}


}

int find_stream(AVFormatContext *fmt_ctx,int *stream_idx, enum AVMediaType type) {
	
	int stream_index;

	stream_index = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (stream_index < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(type), src_filename);
		
	} else {
		*stream_idx = stream_index;
	}
	return stream_index;
}

/*
每个stream对应一个codecContext,每个codecContext里面有codec
*/
int open_stream_codec_context(AVFormatContext* fmt_ctx, AVCodecContext* dec_ctx, int stream_idx) {
	AVStream* st;
	AVCodec* dec = NULL;
	int ret = 0;

	st = fmt_ctx->streams[stream_idx];

	/* find decoder for the stream */
	dec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!dec) {
		fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(st->codecpar->codec_type));
		return AVERROR(EINVAL);
	}

	/* Allocate a codec context for the decoder */
	dec_ctx = avcodec_alloc_context3(dec);
	if (!dec_ctx) {
		fprintf(stderr, "Failed to allocate the %s codec context\n", av_get_media_type_string(dec->type));
		return AVERROR(ENOMEM);
	}

	/* Copy codec parameters from input stream to output codec context */
	if ((ret = avcodec_parameters_to_context(dec_ctx, st->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n", av_get_media_type_string(dec_ctx->codec->type));
		return ret;
	}

	if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
		fprintf(stderr, "Failed to open %s codec\n",av_get_media_type_string(dec->type));
		return ret;
	}

	return ret;
}

int decode_packet(AVCodecContext* dec_ctx,AVPacket *pkt,AVFrame *frame) {
	int ret;
	
	ret = avcodec_send_packet(dec_ctx,pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		return ret;
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return ret;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			return ret;
		}
		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);
	}

	return 0;

}

void Store_frame(AVFrame* frame, const char* filename) {

	char buf[1024];
	FILE* f;
	/* the picture is allocated by the decoder. no need to free it */
	snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);

	f = fopen(buf,"w");
	fprintf(f, "P5\n%d %d\n%d\n", frame->width, frame->height, 255);
	fclose(f);
}

void main() {
	printf("Main start%s",avcodec_configuration());
	int ret = 0;

	model_demuxing();

	if (find_stream(fmt_ctx, &video_stream_idx, AVMEDIA_TYPE_VIDEO) < 0) {
		exit(1);
	}

	if (open_stream_codec_context(fmt_ctx, dec_ctx, video_stream_idx) < 0) {
		exit(1);
	}

	/* dump input information to stderr */
	av_dump_format(fmt_ctx, 0, src_filename, 0);




	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	int i = 0;
	while (av_read_frame(fmt_ctx,&pkt) == 0) {
		if (pkt.stream_index == NULL)
			continue;
		printf("==%d", pkt.stream_index);
		//if (pkt.stream_index == video_stream_idx &&  0 == decode_packet(dec_ctx, &pkt, frame)) {
		//	Store_frame(frame,outfilename);
		//}
		//printf("==== %d",i++);
		//while (1);
	}


	/* flush the decoder */
	//decode_packet(dec_ctx, NULL, frame);

	//fclose(f);

	//av_parser_close(parser);
	//avcodec_free_context(&c);
	//av_frame_free(&frame);
	//av_packet_free(&pkt);

	printf("Main end");
	//while (1);


}