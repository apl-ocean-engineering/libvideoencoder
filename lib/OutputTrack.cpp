extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
  #include <libavutil/opt.h>
  #include <libavutil/error.h>
}

#include <iostream>
using namespace std;

#include <assert.h>

#include "libvideoencoder/OutputTrack.h"

typedef struct AVCodecTag {
    enum AVCodecID id;
    unsigned int tag;
} AVCodecTag;


namespace libvideoencoder {


  static AVFrame *alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
  {
      AVFrame *picture;
      int ret;
      picture = av_frame_alloc();
      if (!picture)
          return NULL;
      picture->format = pix_fmt;
      picture->width  = width;
      picture->height = height;
      /* allocate the buffers for the frame data */
      ret = av_frame_get_buffer(picture, 32);
      if (ret < 0) {
          fprintf(stderr, "Could not allocate frame data.\n");
          exit(1);
      }
      return picture;
  }



  OutputTrack::OutputTrack( AVFormatContext *oc )
        : _stream( avformat_new_stream( oc, NULL ) )
  {
    assert( _stream != nullptr );
    _stream->id = oc->nb_streams-1;
  }

  OutputTrack::~OutputTrack()
  {
  }


  //====================================================================================

  VideoTrack::VideoTrack( AVFormatContext *oc, AVCodec *codec, int width, int height, float frameRate )
  : OutputTrack( oc ),
    _enc( avcodec_alloc_context3(codec) ),
    _numSamples(0),
    _scaledFrame(nullptr),
    _encodedData( av_buffer_alloc(10000000) ),
    _swsCtx(nullptr)
  {
    assert( _enc != nullptr );

    _enc->frame_number = 0;
    _enc->codec_type = AVMEDIA_TYPE_VIDEO;
    _enc->codec_id = codec->id;
    _enc->width    = width;
    _enc->height   = height;

    _enc->time_base.den = int(frameRate * 100.0);
    _enc->time_base.num = 100;

    _stream->time_base = _enc->time_base;
    _stream->avg_frame_rate.num = int(frameRate*100);
    _stream->avg_frame_rate.den = 100;

    {
      const enum AVPixelFormat *pixFmt = codec->pix_fmts;
      // cout << "This codec supports these pixel formats: ";
      // for( int i = 0; pixFmt[i] != -1; i++ ) {
      //   cout << pixFmt[i] << " ";
      // }
      // cout << std::endl;
      _enc->pix_fmt = pixFmt[0];
    }

    // //st->codecpar->format = AV_PIX_FMT_YUV420P;
    if (_enc->codec_id == AV_CODEC_ID_PRORES ) {

      // For ProRes
      // 0 = Proxy
      // 1 = LT
      // 2 = normal
      // 3 = HQ
      _enc->profile = 2; //PRORES_PROFILE_STANDARD;

      const int qscale = 4;
      _enc->flags |= AV_CODEC_FLAG_QSCALE;
      _enc->global_quality = FF_QP2LAMBDA * qscale;

      // auto res = av_opt_set_int( _enc->priv_data, "bits_per_mb", 8192, 0);
      // if(res != 0) {
      //   char buf[80];
      //   av_strerror(res, buf, 79);
      //   std::cerr << "Error setting option \"bits_per_mb\": " << buf << std::endl;
      // }

      auto res = av_opt_set( _enc->priv_data, "profile", "standard", 0);
      if(res != 0) {
        char buf[80];
        av_strerror(res, buf, 79);
        std::cerr << "Error setting option \"bits_per_mb\": " << buf << std::endl;
      }

      // dumpEncoderOptions( _enc );

    }

    {
      auto ret = avcodec_open2(_enc, _enc->codec, nullptr);

      if (ret < 0) {
        std::cerr << "Could not open video codec"; //: " <<  av_err2str(ret) << std::endl;
      }

    }

    {
      auto ret = avcodec_parameters_from_context(_stream->codecpar, _enc);
      if (ret < 0) {
        //fprintf(stderr, "Could not copy the Track parameters\n");
      }
    }

    std::cout << "Codec time base: " << _enc->time_base.num << "/" << _enc->time_base.den << std::endl;
    std::cout << "Video Track time base: " << _stream->time_base.num << "/" << _stream->time_base.den << std::endl;

  }

  VideoTrack::~VideoTrack()
  {
    if( _swsCtx ) sws_freeContext( _swsCtx );
    if( _scaledFrame ) av_frame_free( &_scaledFrame );
    if( _enc )      avcodec_close( _enc );
    //if( _stream )  avformat_free_context( &st );  // Track is destroyed when AVFormatContext is cleaned up
  }


  AVPacket *VideoTrack::addFrame( AVFrame *frame, int frameNum )
  {

    if ( !frame || !frame->data[0]) return nullptr;

    // Lazy-create the swscaler RGB to YUV420P.
    if (!_swsCtx) {
      _swsCtx = sws_getContext(_enc->width, _enc->height,
                                (AVPixelFormat)frame->format,              // Assume frame format will be consistent...
                                _enc->width, _enc->height,
                                _enc->pix_fmt,
                                SWS_BICUBLIN, NULL, NULL, NULL);
      assert( _swsCtx );
    }

    frame->pts = frameNum;

    if ( _enc->pix_fmt != (AVPixelFormat)frame->format )	{

      if( !_scaledFrame ) {
        // Lazy-allocate frame if you're going to be scaling
        _scaledFrame = alloc_frame(_enc->pix_fmt, _enc->width, _enc->height);
        assert(_scaledFrame);
      }

      // Convert RGB to YUV.
      auto res = sws_scale(_swsCtx, frame->data, frame->linesize, 0,
                            _enc->height, _scaledFrame->data, _scaledFrame->linesize);

      _scaledFrame->pts = frame->pts;
      return encode(_scaledFrame);
    }

    return encode(frame);
  }


  AVPacket *VideoTrack::encode( AVFrame *frame ) {

    assert( _encodedData );

    // Encode
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);

    packet->buf = av_buffer_ref(_encodedData);
    packet->size = _encodedData->size;
    packet->data = NULL;

    int nOutputSize = 0;

    // Encode frame to packet->
    // Todo:  switch to avcodec_send_frame / avcodec_receive_packet
    auto result = avcodec_encode_video2(_enc, packet, frame, &nOutputSize);

    packet->stream_index = _stream->index;
    packet->pts          = frame->pts;
    packet->dts          = frame->pts;

    if ((result < 0) || (nOutputSize <= 0)) {
      std::cerr << "Error encoding video" << std::endl;
      return nullptr;
    }

    av_packet_rescale_ts( packet, _enc->time_base, _stream->time_base );

    _numSamples++;

    return packet;
  }


  void VideoTrack::dumpEncoderOptions( AVCodecContext *enc ) {

    {
      // Query the results!
      int64_t val = 0;
      auto res = av_opt_get_int( enc->priv_data, "qscale", 0, &val );
      if( res == 0 ) {
        std::cerr << "QScale set to " << val << endl;
      } else {
        char buf[80];
        av_strerror(res, buf, 79);
        std::cerr << "Error querying option \"qscale\": " << buf << std::endl;
      }

      val = 0;
      res = av_opt_get_int( enc->priv_data, "bits_per_mb", 0, &val );
      if( res == 0 ) {
        std::cerr << "bits_per_mb set to " << val << endl;
      } else {
        char buf[80];
        av_strerror(res, buf, 79);
        std::cerr << "Error querying option \"bits_per_mb\": " << buf << std::endl;
      }

      unsigned char *out;
      av_opt_get( enc->priv_data, "profile", 0, &out );
      if( res == 0 ) {
        std::cerr << "Profile set to " << (char *)out << endl;
      } else {
        char buf[80];
        av_strerror(res, buf, 79);
        std::cerr << "Error querying option \"profile\": " << buf << std::endl;
      }
    }
  }


  //====================================================================================

  DataTrack::DataTrack( AVFormatContext *oc )
  : OutputTrack( oc ),
    _numSamples(0)
  {
    // DON'T USE _enc

    _stream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
    _stream->codecpar->codec_id = AV_CODEC_ID_BIN_DATA; //AV_CODEC_ID_NONE;
    _stream->codecpar->codec_tag = MKTAG('g', 'p', 'm', 'd');
    //_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    // Base time is microseconds?
    _stream->time_base.den = 1000000;
    _stream->time_base.num = 1;


    const struct AVCodecTag* const* codec_tags = oc->oformat->codec_tag;
    for( int i = 0; codec_tags[i] && codec_tags[i]->id != AV_CODEC_ID_NONE; ++i ) {
      cerr << "ID: " << codec_tags[i]->id << " ; tag: " << std::hex << codec_tags[i]->tag << endl;
    }

    //std::cout << "Codec time base: " << _enc->time_base.num << "/" << _enc->time_base.den << std::endl;
    std::cout << "Data Track time base: " << _stream->time_base.num << "/" << _stream->time_base.den << std::endl;

  }

  DataTrack::~DataTrack()
  {
  }

    //
    // AVPacket *DataTrack::addFrame( AVFrame *frame, int frameNum )
    // {
    //
    //   if ( !frame || !frame->data[0]) return nullptr;
    //
    // }

    //   // Lazy-create the swscaler RGB to YUV420P.
    //   if (!_swsCtx) {
    //     _swsCtx = sws_getContext(_enc->width, _enc->height,
    //                               (AVPixelFormat)frame->format,              // Assume frame format will be consistent...
    //                               _enc->width, _enc->height,
    //                               _enc->pix_fmt,
    //                               SWS_BICUBLIN, NULL, NULL, NULL);
    //     assert( _swsCtx );
    //   }
    //
    //   frame->pts = frameNum;
    //
    //   if ( _enc->pix_fmt != (AVPixelFormat)frame->format )	{
    //
    //     if( !_scaledFrame ) {
    //       // Lazy-allocate frame if you're going to be scaling
    //       _scaledFrame = alloc_frame(_enc->pix_fmt, _enc->width, _enc->height);
    //       assert(_scaledFrame);
    //     }
    //
    //     // Convert RGB to YUV.
    //     auto res = sws_scale(_swsCtx, frame->data, frame->linesize, 0,
    //                           _enc->height, _scaledFrame->data, _scaledFrame->linesize);
    //
    //     _scaledFrame->pts = frame->pts;
    //     return encode(_scaledFrame);
    //   }
    //
    //   return encode(frame);
    // }
    //
    //
    // AVPacket *VideoTrack::encode( AVFrame *frame ) {
    //
    //   assert( _encodedData );
    //
    //   // Encode
    //   AVPacket *packet = new AVPacket;
    //   packet->buf = av_buffer_ref(_encodedData);
    //   packet->size = _encodedData->size;
    //   packet->data = NULL;
    //
    //   int nOutputSize = 0;
    //
    //   // Encode frame to packet->
    //   // Todo:  switch to avcodec_send_frame / avcodec_receive_packet
    //   auto result = avcodec_encode_video2(_enc, packet, frame, &nOutputSize);
    //
    //   packet->Track_index = _stream->index;
    //   packet->pts          = frame->pts;
    //   packet->dts          = frame->pts;
    //
    //   if ((result < 0) || (nOutputSize <= 0)) {
    //     std::cerr << "Error encoding video" << std::endl;
    //     return nullptr;
    //   }
    //
    //   av_packet_rescale_ts( packet, _enc->time_base, _stream->time_base );
    //
    //   _numSamples++;
    //
    //   return packet;
    // }


};
