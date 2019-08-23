#pragma once

#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/avutil.h>
}

#include <mutex>
#include <vector>
#include <string>
#include <memory>

//#include "libvideoencoder/OutputTrack.h"

namespace libvideoencoder {

  class VideoWriter {
  public:

    VideoWriter( const std::string &container, const AVCodecID codec = AV_CODEC_ID_NONE );

    VideoWriter( const std::string &container, const std::string &codec);

    ~VideoWriter();

    void describeCodec( AVCodecID codec_id );

    AVFormatContext *outputFormatContext() { return _outFormatContext; }
    AVCodec *codec()                       { return _codec; }

    // size_t addVideoTrack( const int width, const int height, const float frameRate, int numStreams = 1 );
    // size_t addDataTrack( );

    bool open( const std::string &inputFile );
    bool close();

    // Packets are _not_ encoded
    bool writePacket(AVPacket* pkt );

  private:

    //AVOutputFormat  *_outFormat;
    AVCodec *_codec;
    AVFormatContext *_outFormatContext;

    //std::vector< std::shared_ptr<OutputTrack> > _streams;

    std::mutex _writeMutex;

    // Free all resourses.
    //void Free();

  };

}
