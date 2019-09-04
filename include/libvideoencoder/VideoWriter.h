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

namespace libvideoencoder {

  class VideoWriter {
  public:

    VideoWriter( const std::string &container, const AVCodecID codec = AV_CODEC_ID_NONE );

    VideoWriter( const std::string &container, const std::string &codec);

    ~VideoWriter();

    void describeCodec( AVCodecID codec_id );

    AVFormatContext *outputFormatContext() { return _outFormatContext; }
    AVCodec *codec()                       { return _codec; }

    bool open( const std::string &inputFile );
    bool close();

    // Packets are _not_ encoded
    bool writePacket( AVPacket* pkt );

  private:

    AVCodec *_codec;
    AVFormatContext *_outFormatContext;


    std::mutex _writeMutex;

  };

}
