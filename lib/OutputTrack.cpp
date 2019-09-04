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

  OutputTrack::OutputTrack( VideoWriter &writer )
        : _stream( avformat_new_stream( writer.outputFormatContext(), NULL ) ),
          _numSamples(0),
          _startTime( std::chrono::system_clock::now() )
  {
    assert( _stream != nullptr );
    _stream->id = writer.outputFormatContext()->nb_streams-1;
  }

  OutputTrack::~OutputTrack()
  {
  }


  //====================================================================================

  VideoTrack::VideoTrack( VideoWriter &writer, int width, int height, float frameRate )
  : OutputTrack( writer ),
    _enc( avcodec_alloc_context3(writer.codec()) ),
    _scaledFrame(nullptr),
    _encodedData( av_buffer_alloc(10000000) ),
    _swsCtx(nullptr)
  {
    assert( _enc != nullptr );

    _enc->frame_number = 0;
    _enc->codec_type = AVMEDIA_TYPE_VIDEO;
    _enc->codec_id = writer.codec()->id;
    _enc->width    = width;
    _enc->height   = height;

    _enc->framerate.num = int( frameRate * 1001 );
    _enc->framerate.den = 1001;

    _enc->time_base.num = 1001;
    _enc->time_base.den = int( frameRate * 1001 );

    _stream->time_base = _enc->time_base;
    _stream->avg_frame_rate.den = _enc->time_base.num;
    av_stream_set_r_frame_rate( _stream, _enc->framerate );
    _stream->avg_frame_rate = av_stream_get_r_frame_rate( _stream );



    {
      const enum AVPixelFormat *pixFmt = writer.codec()->pix_fmts;
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

      const int qscale = 6;
      _enc->flags |= AV_CODEC_FLAG_QSCALE;
      _enc->global_quality = FF_QP2LAMBDA * qscale;

      // auto res = av_opt_set_int( _enc->priv_data, "bits_per_mb", 8192, 0);
      // if(res != 0) {
      //   char buf[80];
      //   av_strerror(res, buf, 79);
      //   std::cerr << "Error setting option \"bits_per_mb\": " << buf << std::endl;
      // }

      // auto res = av_opt_set( _enc->priv_data, "profile", "standard", 0);
      // if(res != 0) {
      //   char buf[80];
      //   av_strerror(res, buf, 79);
      //
      //   std::cerr << "Error setting option \"profile\": " << buf << std::endl;
      // }

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
  }


  AVFrame *VideoTrack::allocateFrame(enum AVPixelFormat pix_fmt)
  {
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture) return nullptr;

    picture->format = pix_fmt;
    picture->width  = _enc->width;
    picture->height = _enc->height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);

    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
  }


  AVPacket *VideoTrack::encodeFrame( const cv::Mat &image, int frameNum )
  {
    const auto sz = image.size();
    AVFrame *frame = allocateFrame(AV_PIX_FMT_BGRA);

    // This depends on both the image and the frame being 32bit RGBA
    cv::Mat frameMat( sz.height, sz.width, CV_8UC4, frame->data[0]);

    assert( image.type() == frameMat.type() );
    image.copyTo( frameMat );

    AVPacket *packet = encodeFrame( frame, frameNum );
    av_frame_free( &frame );

    return packet;
  }

  AVPacket *VideoTrack::encodeFrame( AVFrame *frame, int frameNum )
  {
    if ( !frame || !frame->data[0]) return nullptr;

    frame->pts = frameNum;

    // If image size or format don't need to be changed, jump straight
    // to encoding
    if ( _enc->pix_fmt != (AVPixelFormat)frame->format ) {

      // Lazy-allocated the swscaler RGB to YUV420P.
      if (!_swsCtx) {
        _swsCtx = sws_getContext(_enc->width, _enc->height,
                                  (AVPixelFormat)frame->format, // Assume frame format will be consistent for all frames...
                                  _enc->width, _enc->height,
                                  _enc->pix_fmt,
                                  SWS_BICUBLIN, NULL, NULL, NULL);
        assert( _swsCtx );
      }

      if( !_scaledFrame ) {
        // Lazy-allocate frame if you're going to be scaling
        _scaledFrame = allocateFrame(_enc->pix_fmt);
        assert(_scaledFrame);
      }

      // Convert RGB to YUV.
      auto res = sws_scale(_swsCtx, frame->data, frame->linesize, 0,
                            _enc->height, _scaledFrame->data, _scaledFrame->linesize);

      _scaledFrame->pts = frame->pts;

      return encode( _scaledFrame );
    }

    return encode( frame );
  }

  AVPacket *VideoTrack::encode( AVFrame *frame )
  {
    // Encode
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);

    packet->buf = av_buffer_ref(_encodedData);
    packet->size = _encodedData->size;
    packet->data = NULL;

    // Encode frame to packet->
    {
      auto result = avcodec_send_frame( _enc, frame );
      if( result != 0 ) {
        std::cerr << "Error in avcodec_send_frame: " << result << endl;
        return nullptr;
      }
    }

    {
      auto result = avcodec_receive_packet( _enc, packet );
      if( result != 0 ) {
        std::cerr << "Error in avcodec_receive_packet: " << result << endl;
        return nullptr;
      }
    }

    packet->stream_index = _stream->index;
    packet->pts          = frame->pts;
    packet->dts          = frame->pts;

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

  DataTrack::DataTrack( VideoWriter &writer )
  : OutputTrack( writer )
  {
    // DON'T USE _enc

    _stream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
    _stream->codecpar->codec_id = AV_CODEC_ID_BIN_DATA; //AV_CODEC_ID_NONE;
    _stream->codecpar->codec_tag = MKTAG('g', 'p', 'm', 'd');
    //_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    // Base time is microseconds
    _stream->time_base.den = 1000000;
    _stream->time_base.num = 1;


    // const struct AVCodecTag* const* codec_tags = writer.outputFormatContext()->oformat->codec_tag;
    // for( int i = 0; codec_tags[i] && codec_tags[i]->id != AV_CODEC_ID_NONE; ++i ) {
    //   cerr << "ID: " << codec_tags[i]->id << " ; tag: " << std::hex << codec_tags[i]->tag << endl;
    // }
    //
    // //std::cout << "Codec time base: " << _enc->time_base.num << "/" << _enc->time_base.den << std::endl;
    // std::cout << "Data Track time base: " << _stream->time_base.num << "/" << _stream->time_base.den << std::endl;

  }

  DataTrack::~DataTrack()
  {;}

  char *DataTrack::allocate( size_t len )
  {
    return (char *)av_malloc(len);
  }

  AVPacket *DataTrack::encodeData( void *data, size_t len, const std::chrono::time_point< std::chrono::system_clock > time )
  {
    AVPacket *pkt = av_packet_alloc();
    av_packet_from_data(pkt, (uint8_t *)data, len );

    pkt->stream_index = _stream->index;

    std::chrono::microseconds timePt =  std::chrono::duration_cast<std::chrono::microseconds>(time - _startTime);

    pkt->pts = timePt.count();
    pkt->dts = pkt->pts;

    //LOG(WARNING) << "   packet->pts " << pkt->pts;

    return pkt;
  }

};
