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

#include "libvideoencoder/OutputTrack.h"

namespace libvideoencoder {

  class VideoWriter {
  public:

    VideoWriter( AVOutputFormat  *outFormat,
                 AVCodec *_codec );
    ~VideoWriter();

    size_t addVideoTrack( const int width, const int height, const float frameRate, int numStreams = 1 );
    size_t addDataTrack( );

    bool open( const std::string &inputFile );

    bool close();

    // Frames are passed to the OutputStream for encoding
    bool addFrame(AVFrame* frame, unsigned int frameNum, unsigned int stream = 0 );

    // Packets are _not_ encoded
    bool addPacket(AVPacket* pkt );



  private:

    // State received from Encoder
    AVOutputFormat  *_outFormat;
    AVCodec *_codec;

    // int _width, _height;
    // float _frameRate;
    // int _numStreams;

    // State generated in this module
    AVFormatContext *_outFormatContext;

    std::vector< std::shared_ptr<OutputTrack> > _streams;

    std::mutex _writeMutex;

    // Free all resourses.
    void Free();

  };


  class Encoder {
  public:

    Encoder( const std::string &container, const AVCodecID codec = AV_CODEC_ID_NONE );
    ~Encoder();

     VideoWriter *makeWriter();

   protected:

  private:

    AVOutputFormat  *_outFormat;
    AVCodec *_codec;

  };

};
