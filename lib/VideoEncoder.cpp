/*
FFmpeg simple VideoWriter
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <algorithm>

#include <iostream>

#include "libvideoencoder/VideoEncoder.h"

#include "libvideoencoder/utils.h"


namespace libvideoencoder {
  using namespace std;


  Encoder::Encoder( const std::string &container, const AVCodecID codec_id )
    : _outFormat(nullptr), _codec(nullptr)
  {
    av_log_set_level( AV_LOG_VERBOSE );

    avcodec_register_all();
    av_register_all();

    _outFormat = av_guess_format(container.c_str(), NULL, NULL);
    assert(_outFormat != nullptr );  // Should be an exception?

    // cout << "Using container format " << _outFormat->name << " (" << _outFormat->long_name << ")" << endl;
    //
    // const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(codec_id);
    // if( codecDesc ) {
    //   std::cout << "Using codec " << codec_id << ": " << codecDesc->name << " (" << codecDesc->long_name << ")"  << std::endl;
    // } else {
    //   std::cerr << "Could not retrieve codec description for " << codec_id << std::endl;
    //   return false;
    // }

    _codec = avcodec_find_encoder( codec_id );
    assert( _codec != nullptr );   // Should be an exception?

    //avcodec_register( _codec );
  }


  Encoder::~Encoder()
  {;}


  VideoWriter *Encoder::makeWriter( const int width, const int height, const float frameRate, int numStreams )
  {
    return new VideoWriter( _outFormat, _codec, width, height, frameRate, numStreams );
  }

  //============================================================================================

  VideoWriter::VideoWriter( AVOutputFormat  *outFormat,
                             AVCodec *codec,
                             const int width, const int height, const float frameRate, int numStreams )
  : _codec( codec ),
    _width(width), _height(height),
    _frameRate( frameRate ), _numStreams(numStreams),
    _outFormatContext( avformat_alloc_context() ),
    _vosts()
  {
    _outFormatContext->oformat = outFormat;

    // Add video streams
    for( int i = 0; i < _numStreams; i++ ) {
      _vosts.push_back( shared_ptr<OutputStream>(new OutputStream( _outFormatContext, _codec, _width, _height, _frameRate )) );
    }

  }


  VideoWriter::~VideoWriter()
  {
    close();

    if (_outFormatContext) av_free(_outFormatContext);
  }


  bool VideoWriter::open( const std::string &inputFile )
  //, const std::string &container, const AVCodecID codec, int numStreams)
  {
    assert( _outFormatContext != nullptr );

    //memcpy(_outFormatContext->filename, inputFile.c_str(), std::min(inputFile.size(), sizeof(_outFormatContext->filename)));

    av_dump_format(_outFormatContext, 0, inputFile.c_str(), 1);

    if (avio_open(&_outFormatContext->pb, inputFile.c_str(), AVIO_FLAG_WRITE) < 0)	{
      cerr << "Cannot open file" << endl;
      //Free();
      return false;
    }

    return ( avformat_write_header(_outFormatContext, NULL) == 0 );
  }



  bool VideoWriter::close()
  {
    if(_outFormatContext) {
      cerr << "Writing trailer" << endl;
      av_write_trailer(_outFormatContext);
    }

    if (!(_outFormatContext->flags & AVFMT_NOFILE) && _outFormatContext->pb) {
      avio_close(_outFormatContext->pb);
    }

    return true;
  }


  bool VideoWriter::addFrame(AVFrame* frame, unsigned int frameNum, unsigned int stream )
  {
    auto packet = _vosts.at(stream)->addFrame(frame, frameNum);

    if( !packet ) {
      return false;
    }

    auto result = av_interleaved_write_frame(_outFormatContext, packet);
    delete packet;

    if( result < 0 ) {
      std::cerr << "Error writing interleaved frame" << std::endl;
      return false;
    }

    return true;

  }








    };
