#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
}

#include <assert.h>


namespace libvideoencoder {

  struct OutputTrack {

    OutputTrack( AVFormatContext *oc );

    virtual ~OutputTrack();

    virtual AVPacket *addFrame( AVFrame *frame, int frameNum ) = 0;

  protected:

    AVStream *_stream;

  };


  struct VideoTrack : public OutputTrack {

    VideoTrack( AVFormatContext *oc, AVCodec *codec, int width, int height, float frameRate );

    virtual ~VideoTrack();

    virtual AVPacket *addFrame( AVFrame *frame, int frameNum );

    AVPacket *encode( AVFrame *frame );

  protected:

    void dumpEncoderOptions( AVCodecContext *enc );


    AVCodecContext *_enc;

    /* pts of the next frame that will be generated */
    // int64_t next_pts;
    int _numSamples;

    AVFrame *_scaledFrame;
    AVBufferRef *_encodedData;
    //AVFrame *tmp_frame;

    // float t, tincr, tincr2;
    struct SwsContext *_swsCtx;
    // struct SwrContext *swr_ctx;
  };




  struct DataTrack : public OutputTrack {

    DataTrack( AVFormatContext *oc );

    virtual ~DataTrack();

    virtual AVPacket *addFrame( AVFrame *frame, int frameNum ) { return nullptr; }

  protected:

    /* pts of the next frame that will be generated */
    // int64_t next_pts;
    int _numSamples;


    // struct SwrContext *swr_ctx;
  };

};
