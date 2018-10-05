#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

#include <string>

#include "VideoWriter.h"

namespace libvideoencoder {

  class Encoder {
  public:

    Encoder( const std::string &container, const AVCodecID codec = AV_CODEC_ID_NONE );
    Encoder( const std::string &container, const std::string &codec);

    ~Encoder();

     VideoWriter *makeWriter();

  protected:

    void describeCodec( AVCodecID codec_id );

  private:

    AVOutputFormat  *_outFormat;
    AVCodec *_codec;

  };

};
