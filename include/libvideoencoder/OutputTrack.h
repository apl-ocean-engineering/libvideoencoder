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

namespace libvideoencoder {

  struct OutputTrack {

    OutputTrack( VideoWriter &writer );

    virtual ~OutputTrack();

    int streamNum() const { return _stream->index; }

  protected:

    VideoWriter &_writer;

    AVStream *_stream;

    std::chrono::time_point< std::chrono::system_clock > _startTime;

  };


  struct VideoTrack : public OutputTrack {

    VideoTrack( VideoWriter &writer, int width, int height, float frameRate );

    virtual ~VideoTrack();

    bool addFrame( AVFrame *frame, int frameNum );

    AVFrame *makeFrame(enum AVPixelFormat pix_fmt = AV_PIX_FMT_RGB24 );

  protected:

    bool encode( AVFrame *frame );

    void dumpEncoderOptions( AVCodecContext *enc );

    AVCodecContext *_enc;

    /* pts of the next frame that will be generated */
    // int64_t next_pts;
    int _numSamples;

    AVFrame *_scaledFrame;
    AVBufferRef *_encodedData;

    struct SwsContext *_swsCtx;
  };




  struct DataTrack : public OutputTrack {

    DataTrack( VideoWriter &writer );

    virtual ~DataTrack();

    char *alloc( size_t len );
    bool writeData( void *data, size_t len, int64_t pts = 0 );

  protected:

    /* pts of the next frame that will be generated */
    // int64_t next_pts;
    int _numSamples;


    // struct SwrContext *swr_ctx;
  };

};
