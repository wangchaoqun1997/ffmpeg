
extern "C" {


#include <libavformat\avformat.h>
}




const char* outfileURL = "rtsp://192.168.2.73/yyy";
const char* outfileFormat = "rtsp";
const int outWidth = 680;
const int outHeight = 480;
const int outBitRate = 400000;
const AVPixelFormat outPixelFormal = AV_PIX_FMT_YUV420P;

#define STREAM_FRAME_RATE 25
const AVRational outTimeBase = {1,STREAM_FRAME_RATE };

AVFormatContext* aVFormatContext;
AVOutputFormat* aVOutputFormat;

AVCodec* videoCodec,*audioCodec;
bool isHaveVideoStream=false, isHaveAudioStream = false;
typedef struct OutputStream {
	AVStream* st;
	AVCodecContext* enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame* frame;
	AVFrame* tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext* sws_ctx;
	struct SwrContext* swr_ctx;
} OutputStream;
OutputStream videoputStream,audioputStream;
// a wrapper around a single output AVStream


int Step1_AVFormatContext() {

	avformat_alloc_output_context2(&aVFormatContext, NULL, outfileFormat, outfileURL);

	if (aVFormatContext == NULL) {
		return -1;
	}

	aVOutputFormat = aVFormatContext->oformat;

	return 0;
}

int Step2_AddStream(OutputStream* ost, AVFormatContext* oc, AVCodec** codec, AVCodecID codec_id) {
	
	AVCodecContext* avCodecContext;

	*codec = avcodec_find_encoder(codec_id);
	avCodecContext = avcodec_alloc_context3(*codec);


	ost->st = avformat_new_stream(oc, NULL);
	ost->st->id = oc->nb_streams - 1;
	ost->enc = avCodecContext;
	ost->st->time_base = outTimeBase;

	if ((*codec)->type == AVMEDIA_TYPE_VIDEO) {
		
	}
	else if((*codec)->type == AVMEDIA_TYPE_AUDIO) {
		avCodecContext->codec_id = codec_id;
		avCodecContext->bit_rate = outBitRate;
		avCodecContext->width = outWidth;
		avCodecContext->height = outHeight;
		avCodecContext->time_base = outTimeBase;
		avCodecContext->gop_size = 12;
		avCodecContext->pix_fmt = outPixelFormal;

		if (avCodecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B-frames */
			avCodecContext->max_b_frames = 2;
		}
		if (avCodecContext->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			 * This does not happen with normal video, it just happens here as
			 * the motion of the chroma plane does not match the luma plane. */
			avCodecContext->mb_decision = 2;
		}
	}


	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	return 0;
}

AVFrame * alloc_picture(AVPixelFormat pix_fmt,int width,int height) {
	
	int ret;
	AVFrame* picture = av_frame_alloc();
	if (picture == NULL)
		return NULL;
	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	ret = av_frame_get_buffer(picture,32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate frame data.\n");
		exit(1);
	}

	return picture;
}

int Step3_OpenVideo(OutputStream* ost, AVDictionary *opt_arg) {
	int ret;
	AVCodecContext* c = ost->enc;

	AVDictionary* opt = NULL;
	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c,videoCodec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		//fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
		exit(1);
	}
	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt,c->width,c->height);
	if (!ost->frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
 * picture is needed too. It is then converted to the required
 * output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			fprintf(stderr, "Could not allocate temporary picture\n");
			exit(1);
		}
	}
	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}

}
int main() {

	int ret;

	Step1_AVFormatContext();
	
	if (aVFormatContext->video_codec_id != AV_CODEC_ID_NONE) {
		Step2_AddStream(&videoputStream, aVFormatContext, &videoCodec, aVFormatContext->video_codec_id);
		isHaveVideoStream = true;
	}
	else if (aVFormatContext->audio_codec_id != AV_CODEC_ID_NONE) {
		Step2_AddStream(&audioputStream, aVFormatContext, &audioCodec, aVFormatContext->audio_codec_id);
		isHaveAudioStream = true;
	}

	if (isHaveVideoStream) {
		Step3_OpenVideo(&videoputStream,NULL);
	}
	av_dump_format(aVFormatContext, 0, outfileURL, 1);


	/* Write the stream header, if any. */
	ret = avformat_write_header(aVFormatContext,NULL);
	if (ret < 0) {
		//fprintf(stderr, "Error occurred when opening output file: %s\n",
		//	av_err2str(ret));
		return 1;
	}


}


