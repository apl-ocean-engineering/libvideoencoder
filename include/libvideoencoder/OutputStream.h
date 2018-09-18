#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
}

#include <assert.h>

#include <iostream>

#include "libvideoencoder/utils.h"


namespace libvideoencoder {

  struct OutputStream {

    OutputStream( AVFormatContext *oc )
    : _oc(oc), st(nullptr),
      enc(nullptr),
      _numSamples(0),
      _scaledFrame(nullptr),
      _encodedData( av_buffer_alloc(10000000) ),
      _swsCtx(nullptr)
    {}

    ~OutputStream()
    {
      if( _swsCtx ) sws_freeContext( _swsCtx );
      if( _scaledFrame ) av_frame_free( &_scaledFrame );
      if( enc )     avcodec_close( enc );
      //if( st )      avformat_free_context( &st );  // Stream is destroyed when AVFormatContext is cleaned up
    }


  bool init( AVCodecID codec_id, int width, int height, float frameRate ) {

    {
      const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(codec_id);
      if( codecDesc ) {
        std::cout << "Using codec " << codec_id << ": " << codecDesc->name << " (" << codecDesc->long_name << ")"  << std::endl;
      } else {
        std::cerr << "Could not retrieve codec description for " << codec_id << std::endl;
        return false;
      }
    }

    AVCodec *codec = avcodec_find_encoder( codec_id );
    if( codec == nullptr ) {
      std::cerr << "Unable to find encoder codec" << std::endl;
      return false;
    }

    st = avformat_new_stream( _oc, NULL );
    if (!st){
      std::cerr << "Cannot add new video stream" << std::endl;
      return false;
    }
    st->id = _oc->nb_streams-1;

    // Settings params, how does this get passed to the codec?

    //AVCodecParameters *params = st->codecpar;

    // st->codecpar->codec_id = codec_id;
    // st->codecpar->width = W_VIDEO;
    // st->codecpar->height = H_VIDEO;
    //
    // st->time_base.den = 25;
    // st->time_base.num = 1;

    AVCodecContext *c = avcodec_alloc_context3(codec);
    if( !c ) {
      std::cerr << "Cannot create codec context" << std::endl;
      return false;
    }
    enc = c;

    //AVCodecParameters *codecPars = stream->codecpar;
    c->frame_number = 0;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->codec_id = codec_id;
    c->width    = width;
    c->height   = height;

    /* time base: this is the fundamental unit of time (in seconds) in terms
    of which frame timestamps are represented. for fixed-fps content,
    timebase should be 1/framerate and timestamp increments should be
    identically 1. */
    c->time_base.den = int(frameRate * 100.0);
    c->time_base.num = 100;

    st->time_base = c->time_base;

    //pCodecCxt->gop_size = 12; // emit one intra frame every twelve frames at most

    const enum AVPixelFormat *pixFmt = codec->pix_fmts;
    // cout << "This codec supports these pixel formats: ";
    // for( int i = 0; pixFmt[i] != -1; i++ ) {
    //   cout << pixFmt[i] << " ";
    // }
    // cout << std::endl;

    c->pix_fmt = pixFmt[0];

    // //st->codecpar->format = AV_PIX_FMT_YUV420P;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
      // Just for testing, we also add B frames
      //pCodecCxt->max_b_frames = 2;
    } else if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
      /* Needed to avoid using macroblocks in which some coeffs overflow.
      This does not happen with normal video, it just happens here as
      the motion of the chroma plane does not match the luma plane. */
      //pCodecCxt->mb_decision = 2;
    } else if (c->codec_id == AV_CODEC_ID_PRORES ) {

      // For ProRes
      // 0 = Proxy
      // 1 = LT
      // 2 = normal
      // 3 = HQ
      c->profile = 2;
    }

    return true;
  }

  bool open()
  {
    int ret;
    // AVDictionary *opt = NULL;
    // av_dict_copy(&opt, opt_arg, 0);
    AVCodec *codec = avcodec_find_encoder( enc->codec_id );
    if( codec == nullptr ) {
      std::cerr << "Unable to find encoder codec" << std::endl;
      return false;
    }

    ret = avcodec_open2(enc, codec, nullptr);
    //av_dict_free(&opt);
    if (ret < 0) {
      std::cerr << "Could not open video codec"; //: " <<  av_err2str(ret) << std::endl;
      return false;;
    }

    ret = avcodec_parameters_from_context(st->codecpar, enc);
    if (ret < 0) {
      //fprintf(stderr, "Could not copy the stream parameters\n");
      return false;
    }

    std::cout << "Codec time base: " << enc->time_base.num << "/" << enc->time_base.den << std::endl;
    std::cout << "Video stream time base: " << st->time_base.num << "/" << st->time_base.den << std::endl;

    return true;
  }


  bool addFrame( AVFrame *frame, int frameNum )
  {
      // Lazy-create the swscaler RGB to YUV420P.
      if (!_swsCtx) {
        _swsCtx = sws_getContext(enc->width, enc->height,
                                  (AVPixelFormat)frame->format,              // Assume frame format will be consistent...
                                  enc->width, enc->height,
                                  enc->pix_fmt,
                                  SWS_BICUBLIN, NULL, NULL, NULL);
        assert( _swsCtx );
      }

      frame->pts = frameNum;

      if ( enc->pix_fmt != (AVPixelFormat)frame->format )	{

        if( !_scaledFrame ) {
          // Lazy-allocate frame if you're going to be scaling
          _scaledFrame = alloc_frame(enc->pix_fmt, enc->width, enc->height);
          assert(_scaledFrame);
        }

        // Convert RGB to YUV.
        auto res = sws_scale(_swsCtx, frame->data, frame->linesize, 0,
                              enc->height, _scaledFrame->data, _scaledFrame->linesize);

        _scaledFrame->pts = frame->pts;
        return encode(_scaledFrame);
      }


      return encode(frame);
    }


    bool encode( AVFrame *frame ) {

      assert( _encodedData );

      // Encode
      AVPacket packet;
      packet.buf = av_buffer_ref(_encodedData);
      packet.size = _encodedData->size;
      packet.data = NULL;

      int nOutputSize = 0;

      // Encode frame to packet.
      // Todo:  switch to avcodec_send_frame / avcodec_receive_packet
      auto result = avcodec_encode_video2(enc, &packet, frame, &nOutputSize);

      packet.stream_index = st->index;
      packet.pts          = frame->pts;
      packet.dts          = frame->pts;

      if ((result < 0) || (nOutputSize <= 0)) {
        std::cerr << "Error encoding video" << std::endl;
        return false;
      }

      av_packet_rescale_ts( &packet, enc->time_base, st->time_base );

      result = av_interleaved_write_frame(_oc, &packet);
      if( result < 0 ) {
        std::cerr << "Error writing interleaved frame" << std::endl;
        return false;
      }

      _numSamples++;

      return true;
    }

  protected:

    AVFormatContext *_oc;

    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    // int64_t next_pts;
    int _numSamples;

    AVFrame *_scaledFrame;
    AVBufferRef *_encodedData;
    //AVFrame *tmp_frame;

    // float t, tincr, tincr2;
    struct SwsContext *_swsCtx;

    // struct SwrContext *swr_ctx;
  };

};
