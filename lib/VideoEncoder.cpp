/*
FFmpeg simple VideoWriter
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <algorithm>
#include <ctime>

#include <iostream>

extern "C" {
  #include <libavutil/dict.h>
}

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
  }


  Encoder::~Encoder()
  {;}


  VideoWriter *Encoder::makeWriter( )
  {
    return new VideoWriter( _outFormat, _codec );
  }


};
