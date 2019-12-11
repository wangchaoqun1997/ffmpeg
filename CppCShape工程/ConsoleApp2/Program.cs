using FFmpeg.AutoGen;
using System;

namespace ConsoleApp2
{
    unsafe class Program
    {
        FFmpegAPI ffmpegobj;

        //string filename = "rtsp://192.168.2.73/yyy";
        string filename = "xxx.mp4";
        AVFormatContext* oc = null;
        AVOutputFormat* fmt;
        int have_video = 0, have_audio = 0;
        int encode_video = 0, encode_audio = 0;
        FFmpegAPI.OutputStream video_st, audio_st;
        AVCodec* audio_codec;
        AVCodec* video_codec;
        AVDictionary* opt = null;
        int ret;

        void Init() {
            ffmpegobj = new FFmpegAPI();

            fixed (AVFormatContext** xx = &oc)
            {
                ffmpeg.avformat_alloc_output_context2(xx, null, "mp4", filename);
                Console.Write("========");
            }
            if (null == oc)
            {
                //Debug.Log("AVFormatContext Null !");
                return;
            }

            fmt = oc->oformat;
            /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
            if (fmt->video_codec != AVCodecID.AV_CODEC_ID_NONE)
            {
                fixed (FFmpegAPI.OutputStream* xx = &video_st)
                {
                    fixed (AVCodec** yy = &video_codec)
                    {
                        ffmpegobj.add_stream(xx, oc, yy, fmt->video_codec);
                        have_video = 1;
                        encode_video = 1;
                    }
                }
            }
            /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
            if (have_video == 1)
            {
                fixed (FFmpegAPI.OutputStream* xx = &video_st)
                {
                    ffmpegobj.open_video(oc, video_codec, xx, opt);
                }
            }
            ffmpeg.av_dump_format(oc, 0, filename, 1);

            ///* Write the stream header, if any. */
            //fixed (AVDictionary** xx = &opt)
            //{
            //    ret = ffmpeg.avformat_write_header(oc, xx);
            //    //if (ret < 0)
            //    //{
            //    //    return;
            //    //}
            //}


            // ("===Start END");
        }

        static void Main(string[] args)
        {
            Program p = new Program();

            p.Init();

            
        }
    }
}
