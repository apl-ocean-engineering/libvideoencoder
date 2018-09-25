
#include <gtest/gtest.h>

#include <string>
#include <iostream>
using namespace std;

#include "libvideoencoder/VideoEncoder.h"
using libvideoencoder::Encoder;
using libvideoencoder::VideoWriter;

static void setPixel(AVFrame *pFrame, short x, short y, short red, short green, short blue ) {

    unsigned int off = (y * pFrame->linesize[0]) + (3*x);

    pFrame->data[0][off] = (uint8_t)red;
    pFrame->data[0][off + 1] = (uint8_t)green;
    pFrame->data[0][off + 2] = (uint8_t)blue;

}


static void fillFrame( AVFrame *frame, unsigned int width, unsigned int height ) {
  for( unsigned int x = 0; x < width; ++x  ) {
    for( unsigned int y = 0; y < height; ++y ) {
      setPixel( frame, x, y, rand() % 256, rand() % 256, rand() % 256  );
    }
  }
}

TEST(TestMakeSampleTwoTrackVideo, goodData) {

  const int Width=320, Height=240;
  const int NumStreams = 2;
  const float FrameRate = 30.0;

  shared_ptr<Encoder> encoder( new Encoder( "mov", AV_CODEC_ID_PRORES ) );
  shared_ptr<VideoWriter> writer( encoder->makeWriter(Width, Height, FrameRate, NumStreams) );

  ASSERT_TRUE( writer->open( "/tmp/test.mov" ) ) << "Unable to initialize encoder.";

  AVFrame *frame = av_frame_alloc();   ///avcodec_alloc_frame();
	ASSERT_NE( frame, nullptr )	<< "Cannot create frame" ;

  frame->width = Width;
  frame->height = Height;
  frame->format = AV_PIX_FMT_RGB24;
	auto res = av_frame_get_buffer(frame, 0);
	ASSERT_GE(res, 0) << "Cannot allocate buffer";

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( int s = 0; s < NumStreams; s++ ) {
      fillFrame( frame, Width, Height );

      writer->addFrame( frame, frameNum, s );
    }
  }

  av_frame_free( &frame );

}
