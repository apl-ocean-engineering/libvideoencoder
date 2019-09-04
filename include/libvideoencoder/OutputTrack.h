#pragma once

#include <chrono>

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
}

#include <assert.h>

#include <libvideoencoder/VideoWriter.h>

#include <opencv2/core.hpp>

namespace libvideoencoder {

  struct OutputTrack {

    OutputTrack( VideoWriter &writer );

    virtual ~OutputTrack();

    int streamNum() const { return _stream->index; }

  protected:

    AVStream *_stream;

    int _numSamples;

    std::chrono::time_point< std::chrono::system_clock > _startTime;

  };


  struct VideoTrack : public OutputTrack {

    VideoTrack( VideoWriter &writer, int width, int height, float frameRate );

    virtual ~VideoTrack();

    // Allocates an AVFrame based on the track's size
    // for writing
    AVFrame *allocateFrame(enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24 );

    AVPacket *encodeFrame( const cv::Mat &image, int frameNum );
    AVPacket *encodeFrame( AVFrame *frame, int frameNum );

  protected:

    AVPacket *encode( AVFrame *frame );

    void dumpEncoderOptions( AVCodecContext *enc );

    AVCodecContext *_enc;

    AVFrame *_scaledFrame;
    AVBufferRef *_encodedData;

    struct SwsContext *_swsCtx;
  };




  struct DataTrack : public OutputTrack {

    DataTrack( VideoWriter &writer );

    virtual ~DataTrack();

    char *allocate( size_t len );

    AVPacket *encodeData( void *data, size_t len,
                    const std::chrono::time_point< std::chrono::system_clock > time = std::chrono::system_clock::now() );

  protected:

  };

};
