using FFmpeg.AutoGen;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

public unsafe class FFmpegAPI
{

    long STREAM_DURATION = 1000l;
    int STREAM_FRAME_RATE = 25; /* 25 images/s */
    AVPixelFormat STREAM_PIX_FMT = AVPixelFormat.AV_PIX_FMT_YUV420P; /* default pix_fmt */

    int SCALE_FLAGS = ffmpeg.SWS_BICUBIC;

    public struct OutputStream
    {
        public AVStream* st;
        public AVCodecContext* enc;

        /* pts of the next frame that will be generated */
        public Int64 next_pts;
        public int samples_count;

        public AVFrame* frame;
        public AVFrame* tmp_frame;

        public float t, tincr, tincr2;
        public SwsContext* sws_ctx;
        public SwrContext* swr_ctx;
    }

    //OutputStream* video_st;
    //OutputStream* audio_st;
    //char* filename;
    //AVOutputFormat* fmt;
    //AVFormatContext* oc;
    //AVCodec* audio_codec;
    //AVCodec* video_codec;
    //int ret;
    //int have_video = 0, have_audio = 0;
    //int encode_video = 0, encode_audio = 0;
    //AVDictionary* opt = null;
    //int i;


    public void log_packet(AVFormatContext* fmt_ctx, AVPacket* pkt)
    {

        AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        //printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        //	av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
        //	av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
        //	av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
        //	pkt->stream_index);
    }

    public int write_frame(AVFormatContext* fmt_ctx, AVRational* time_base, AVStream* st, AVPacket* pkt)
    {
        /* rescale output packet timestamp values from codec to stream timebase */
        ffmpeg.av_packet_rescale_ts(pkt, *time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        return ffmpeg.av_interleaved_write_frame(fmt_ctx, pkt);
    }

    /* Add an output stream. */
    public void add_stream(OutputStream* ost, AVFormatContext* oc, AVCodec** codec, AVCodecID codec_id)
    {
        AVCodecContext* c;
        int i;

        /* find the encoder */
        *codec = ffmpeg.avcodec_find_encoder(codec_id);
        if (null == (*codec))
        {
            //Debug.Log("Could not find encoder for " + ffmpeg.avcodec_get_name(codec_id));
            return;
        }

        ost->st = ffmpeg.avformat_new_stream(oc, null);
        if (null == ost->st)
        {
            //Debug.Log("Could not allocate stream\n");
            return;
        }
        ost->st->id = (int)(oc->nb_streams - 1);
        c = ffmpeg.avcodec_alloc_context3(*codec);
        if (null == c)
        {
            //Debug.Log("Could not alloc an encoding context\n");
            return;
        }
        ost->enc = c;

        switch ((*codec)->type)
        {
            //        	case FFmpeg.AutoGen.AVMediaType.AVMEDIA_TYPE_AUDIO:
            //        		c->sample_fmt = (* codec)->sample_fmts? (* codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            //        		c->bit_rate = 64000;
            //        		c->sample_rate = 44100;
            //        		if ((* codec)->supported_samplerates) {
            //        			c->sample_rate = (* codec)->supported_samplerates[0];
            //        			for (i = 0; (* codec)->supported_samplerates[i]; i++) {
            //        				if ((* codec)->supported_samplerates[i] == 44100)
            //        					c->sample_rate = 44100;
            //        			}
            //        		}
            //        		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
            //c->channel_layout = AV_CH_LAYOUT_STEREO;
            //        		if ((* codec)->channel_layouts) {
            //        			c->channel_layout = (* codec)->channel_layouts[0];
            //        			for (i = 0; (* codec)->channel_layouts[i]; i++) {
            //        				if ((* codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
            //        					c->channel_layout = AV_CH_LAYOUT_STEREO;
            //        			}
            //        		}
            //        		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
            //AVRational a;
            //a.num = 1;
            //        		a.den = c->sample_rate;
            //        		ost->st->time_base = a;
            //        		break;

            case FFmpeg.AutoGen.AVMediaType.AVMEDIA_TYPE_VIDEO:
                c->codec_id = codec_id;

                c->bit_rate = 400000;
                /* Resolution must be a multiple of two. */
                c->width = 352;
                c->height = 288;
                /* timebase: This is the fundamental unit of time (in seconds) in terms
                 * of which frame timestamps are represented. For fixed-fps content,
                 * timebase should be 1/framerate and timestamp increments should be
                 * identical to 1. */
                AVRational b;
                b.num = 1;
                b.den = STREAM_FRAME_RATE;
                ost->st->time_base = b;
                c->time_base = ost->st->time_base;

                c->gop_size = 12; /* emit one intra frame every twelve frames at most */
                c->pix_fmt = STREAM_PIX_FMT;
                if (c->codec_id == AVCodecID.AV_CODEC_ID_MPEG2VIDEO)
                {
                    /* just for testing, we also add B-frames */
                    c->max_b_frames = 2;
                }
                if (c->codec_id == AVCodecID.AV_CODEC_ID_MPEG1VIDEO)
                {
                    /* Needed to avoid using macroblocks in which some coeffs overflow.
                     * This does not happen with normal video, it just happens here as
                     * the motion of the chroma plane does not match the luma plane. */
                    c->mb_decision = 2;
                }
                break;

            default:
                break;
        }

        /* Some formats want stream headers to be separate. */
        if ((oc->oformat->flags & ffmpeg.AVFMT_GLOBALHEADER) != 0)
            c->flags |= ffmpeg.AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /**************************************************************/
    /* audio output */

    public AVFrame* alloc_audio_frame(AVSampleFormat sample_fmt, UInt64 channel_layout, int sample_rate, int nb_samples)
    {
        AVFrame* frame = ffmpeg.av_frame_alloc();
        int ret;

        //        	if (!frame) {
        //        		fprintf(stderr, "Error allocating an audio frame\n");
        //exit(1);
        //        	}

        frame->format = (int)sample_fmt;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;

        if (nb_samples != 0)
        {
            ret = ffmpeg.av_frame_get_buffer(frame, 0);
            //        		if (ret< 0) {
            //        			fprintf(stderr, "Error allocating an audio buffer\n");
            //exit(1);
            //        		}
        }

        return frame;
    }

    public void open_audio(AVFormatContext* oc, AVCodec* codec, OutputStream* ost, AVDictionary* opt_arg)
    {
        AVCodecContext* c;
        int nb_samples;
        int ret;
        AVDictionary* opt = null;

        c = ost->enc;

        /* open it */
        ffmpeg.av_dict_copy(&opt, opt_arg, 0);
        ret = ffmpeg.avcodec_open2(c, codec, &opt);
        ffmpeg.av_dict_free(&opt);
        //if (ret < 0)
        //{
        //    //fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        //    exit(1);
        //}

        /* init signal generator */
        ost->t = 0;
        ost->tincr = (float)(2 * ffmpeg.M_PI * 110.0 / c->sample_rate);
        /* increment frequency by 110 Hz per second */
        ost->tincr2 = (float)(2 * ffmpeg.M_PI * 110.0 / c->sample_rate / c->sample_rate);

        if ((c->codec->capabilities & ffmpeg.AV_CODEC_CAP_VARIABLE_FRAME_SIZE) != 0)
            nb_samples = 10000;
        else
            nb_samples = c->frame_size;

        ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
            c->sample_rate, nb_samples);
        ost->tmp_frame = alloc_audio_frame(AVSampleFormat.AV_SAMPLE_FMT_S16, c->channel_layout,
            c->sample_rate, nb_samples);

        /* copy the stream parameters to the muxer */
        ret = ffmpeg.avcodec_parameters_from_context(ost->st->codecpar, c);
        //if (ret < 0)
        //{
        //    fprintf(stderr, "Could not copy the stream parameters\n");
        //    exit(1);
        //}

        /* create resampler context */
        ost->swr_ctx = ffmpeg.swr_alloc();
        //if (!ost->swr_ctx)
        //{
        //    fprintf(stderr, "Could not allocate resampler context\n");
        //    exit(1);
        //}

        /* set options */
        ffmpeg.av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
        ffmpeg.av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
        ffmpeg.av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AVSampleFormat.AV_SAMPLE_FMT_S16, 0);
        ffmpeg.av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
        ffmpeg.av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
        ffmpeg.av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

        /* initialize the resampling context */
        if ((ret = ffmpeg.swr_init(ost->swr_ctx)) < 0)
        {
            //fprintf(stderr, "Failed to initialize the resampling context\n");
            //exit(1);
        }
    }

    /* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
     * 'nb_channels' channels. */
    public AVFrame* get_audio_frame(OutputStream* ost)
    {
        AVFrame* frame = ost->tmp_frame;
        int j, i, v;
        Int16* q = (Int16*)frame->data[0];

        /* check if we want to generate more frames */
        AVRational c;
        c.num = 1;
        c.den = 1;

        if (ffmpeg.av_compare_ts(ost->next_pts, ost->enc->time_base, STREAM_DURATION, c) > 0)
            return null;

        for (j = 0; j < frame->nb_samples; j++)
        {
            v = (int)(Math.Sin(ost->t) * 10000);
            for (i = 0; i < ost->enc->channels; i++)
                *q++ = (short)v;
            ost->t += ost->tincr;
            ost->tincr += ost->tincr2;
        }

        frame->pts = ost->next_pts;
        ost->next_pts += frame->nb_samples;

        return frame;
    }

    /*
     * encode one audio frame and send it to the muxer
     * return 1 when encoding is finished, 0 otherwise
     */
    public int write_audio_frame(AVFormatContext* oc, OutputStream* ost)
    {
        AVCodecContext* c;
        AVPacket pkt; // data and size must be 0;
        AVFrame* frame;
        int ret;
        int got_packet;
        int dst_nb_samples;

        ffmpeg.av_init_packet(&pkt);
        c = ost->enc;

        frame = get_audio_frame(ost);

        if (frame != null)
        {
            /* convert samples from native format to destination codec format, using the resampler */
            /* compute destination number of samples */
            dst_nb_samples = (int)ffmpeg.av_rescale_rnd(ffmpeg.swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples, c->sample_rate, c->sample_rate, AVRounding.AV_ROUND_UP);
            //av_assert0(dst_nb_samples == frame->nb_samples);

            /* when we pass a frame to the encoder, it may keep a reference to it
             * internally;
             * make sure we do not overwrite it here
             */
            ret = ffmpeg.av_frame_make_writable(ost->frame);
            //if (ret < 0)
            //    exit(1);

            /* convert to destination format */
            //ret = ffmpeg.swr_convert(ost->swr_ctx, ost->frame->data, dst_nb_samples, (byte**)(frame->data), frame->nb_samples);
            if (ret < 0)
            {
                //fprintf(stderr, "Error while converting\n");
                //exit(1);
            }
            frame = ost->frame;
            AVRational d;
            d.num = 1;
            d.den = c->sample_rate;
            frame->pts = ffmpeg.av_rescale_q(ost->samples_count, d, c->time_base);
            ost->samples_count += dst_nb_samples;
        }

        ret = ffmpeg.avcodec_encode_audio2(c, &pkt, frame, &got_packet);
        if (ret < 0)
        {
            //fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
            //exit(1);
        }

        if (got_packet != 0)
        {
            ret = write_frame(oc, &c->time_base, ost->st, &pkt);
            if (ret < 0)
            {
                //fprintf(stderr, "Error while writing audio frame: %s\n",
                //	av_err2str(ret));
                // exit(1);
            }
        }

        return (frame != null || got_packet != 0) ? 0 : 1;
    }

    /**************************************************************/
    /* video output */

    public AVFrame* alloc_picture(AVPixelFormat pix_fmt, int width, int height)
    {
        AVFrame* picture;
        int ret;

        picture = ffmpeg.av_frame_alloc();
        if (null == picture)
            return null;

        picture->format = (int)pix_fmt;
        picture->width = width;
        picture->height = height;

        /* allocate the buffers for the frame data */
        ret = ffmpeg.av_frame_get_buffer(picture, 32);
        if (ret < 0)
        {
            //        		fprintf(stderr, "Could not allocate frame data.\n");
            //exit(1);
        }

        return picture;
    }

    public void open_video(AVFormatContext* oc, AVCodec* codec, OutputStream* ost, AVDictionary* opt_arg)
    {
        int ret;
        AVCodecContext* c = ost->enc;
        AVDictionary* opt = null;

        ffmpeg.av_dict_copy(&opt, opt_arg, 0);

        /* open the codec */
        ret = ffmpeg.avcodec_open2(c, codec, &opt);
        ffmpeg.av_dict_free(&opt);
        if (ret < 0)
        {
            //fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
            //exit(1);
        }

        /* allocate and init a re-usable frame */
        ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
        if (null == ost->frame)
        {
            //fprintf(stderr, "Could not allocate video frame\n");
            //exit(1);
        }

        /* If the output format is not YUV420P, then a temporary YUV420P
         * picture is needed too. It is then converted to the required
         * output format. */
        ost->tmp_frame = null;
        if (c->pix_fmt != AVPixelFormat.AV_PIX_FMT_YUV420P)
        {
            ost->tmp_frame = alloc_picture(AVPixelFormat.AV_PIX_FMT_YUV420P, c->width, c->height);
            if (null == ost->tmp_frame)
            {
                //fprintf(stderr, "Could not allocate temporary picture\n");
                //exit(1);
            }
        }

        /* copy the stream parameters to the muxer */
        ret = ffmpeg.avcodec_parameters_from_context(ost->st->codecpar, c);
        if (ret < 0)
        {
            //fprintf(stderr, "Could not copy the stream parameters\n");
            //exit(1);
        }
    }

    /* Prepare a dummy image. */
    public void fill_yuv_image(AVFrame* pict, int frame_index, int width, int height)
    {
        int x, y, i;

        i = frame_index;

        /* Y */
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
                pict->data[0][y * pict->linesize[0] + x] = (byte)(x + y + i * 3);

        /* Cb and Cr */
        for (y = 0; y < height / 2; y++)
        {
            for (x = 0; x < width / 2; x++)
            {
                pict->data[1][y * pict->linesize[1] + x] = (byte)(128 + y + i * 2);
                pict->data[2][y * pict->linesize[2] + x] = (byte)(64 + x + i * 5);
            }
        }
    }

    public AVFrame* get_video_frame(OutputStream* ost)
    {
        AVCodecContext* c = ost->enc;
        AVRational e;
        e.num = 1;
        e.den = 1;

        /* check if we want to generate more frames */
        if (ffmpeg.av_compare_ts(ost->next_pts, c->time_base, STREAM_DURATION, e) > 0)
            return null;

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally; make sure we do not overwrite it here */
        if (ffmpeg.av_frame_make_writable(ost->frame) < 0)
            return null;

        if (c->pix_fmt != AVPixelFormat.AV_PIX_FMT_YUV420P)
        {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (null == ost->sws_ctx)
            {
                ost->sws_ctx = ffmpeg.sws_getContext(c->width, c->height, AVPixelFormat.AV_PIX_FMT_YUV420P,
                    c->width, c->height,
                    c->pix_fmt,
                    SCALE_FLAGS, null, null, null);
                if (null == ost->sws_ctx)
                {
                    //fprintf(stderr,
                    //    "Could not initialize the conversion context\n");
                    //exit(1);
                }
            }
            fill_yuv_image(ost->tmp_frame, (int)(ost->next_pts), c->width, c->height);
            ffmpeg.sws_scale(ost->sws_ctx, (byte*[])ost->tmp_frame->data,
                        ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                        ost->frame->linesize);
        }
        else
        {

            fill_yuv_image(ost->frame, (int)ost->next_pts, c->width, c->height);
        }

        ost->frame->pts = ost->next_pts++;

        return ost->frame;
    }

    /*
     * encode one video frame and send it to the muxer
     * return 1 when encoding is finished, 0 otherwise
     */
    public int write_video_frame(AVFormatContext* oc, OutputStream* ost)
    {
        int ret;
        AVCodecContext* c;
        AVFrame* frame;
        int got_packet = 0;
        AVPacket pkt;

        c = ost->enc;
        frame = get_video_frame(ost);




        ffmpeg.av_init_packet(&pkt);

        /* encode the image */
        ret = ffmpeg.avcodec_encode_video2(c, &pkt, frame, &got_packet);
        if (ret < 0)
        {
            //fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
            //exit(1);
        }


        if (0 != got_packet)
        {

            ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        }
        else
        {
            ret = 0;
        }

        if (ret < 0)
        {
            //fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
            return 1;
        }

        return (null != frame || got_packet != 0) ? 0 : 1;
    }

    public void close_stream(AVFormatContext* oc, OutputStream* ost)
    {
        ffmpeg.avcodec_free_context(&ost->enc);
        ffmpeg.av_frame_free(&ost->frame);
        ffmpeg.av_frame_free(&ost->tmp_frame);
        ffmpeg.sws_freeContext(ost->sws_ctx);
        ffmpeg.swr_free(&ost->swr_ctx);
    }


}