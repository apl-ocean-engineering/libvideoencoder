#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
}

#include <assert.h>

#include <iostream>

#include "libvideoencoder/utils.h"


namespace libvideoencoder {

  struct OutputStream {

    OutputStream( AVFormatContext *oc, AVCodec *codec, int width, int height, float frameRate );

    ~OutputStream();

    bool open();

    AVPacket *addFrame( AVFrame *frame, int frameNum );

    AVPacket *encode( AVFrame *frame );

  protected:

    AVStream *_stream;
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

};
