
#include <gtest/gtest.h>

#include <string>
#include <iostream>
using namespace std;

#include "libvideoencoder/VideoWriter.h"
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


// == Local constants ==

const int Width=320, Height=240;
const int NumStreams = 2;
const float FrameRate = 30.0;
const string Extension("mov");



TEST(TestMakeSampleVideo, oneVideoTracks) {

  shared_ptr<VideoWriter> writer( new VideoWriter( "mov", AV_CODEC_ID_PRORES ) );

  size_t idx = writer->addVideoTrack( Width, Height, FrameRate );

  ASSERT_TRUE( writer->open( "/tmp/test_onevideo.mov" ) ) << "Unable to initialize encoder.";

  AVFrame *frame = av_frame_alloc();   ///avcodec_alloc_frame();
	ASSERT_NE( frame, nullptr )	<< "Cannot create frame" ;

  frame->width = Width;
  frame->height = Height;
  frame->format = AV_PIX_FMT_RGB24;
	auto res = av_frame_get_buffer(frame, 0);
	ASSERT_GE(res, 0) << "Cannot allocate buffer";

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {
    fillFrame( frame, Width, Height );
    writer->addFrame( frame, frameNum );
  }

  av_frame_free( &frame );

}

TEST(TestMakeSampleVideo, twoVideoTracks) {

  shared_ptr<VideoWriter> writer( new VideoWriter( "mov", AV_CODEC_ID_PRORES ) );

  size_t idx = writer->addVideoTrack( Width, Height, FrameRate, NumStreams );

  ASSERT_TRUE( writer->open( "/tmp/test_twovideo.mov" ) ) << "Unable to initialize encoder.";

  AVFrame *frame = av_frame_alloc();   ///avcodec_alloc_frame();
	ASSERT_NE( frame, nullptr )	<< "Cannot create frame" ;

  frame->width = Width;
  frame->height = Height;
  frame->format = AV_PIX_FMT_RGB24;
	auto res = av_frame_get_buffer(frame, 0);
	ASSERT_GE(res, 0) << "Cannot allocate buffer";

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( size_t s = idx; s < idx+NumStreams; s++ ) {
      fillFrame( frame, Width, Height );

      writer->addFrame( frame, frameNum, s );
    }
  }

  av_frame_free( &frame );

}


TEST(TestMakeSampleVideo, twoVideoOneDataTrack) {

  shared_ptr<VideoWriter> writer( new VideoWriter( "mov", AV_CODEC_ID_PRORES ) );

  size_t idx = writer->addVideoTrack( Width, Height, FrameRate, NumStreams );
  size_t dataTrk = writer->addDataTrack();

  ASSERT_TRUE( writer->open( "/tmp/test_twovideo_onedata." + Extension ) ) << "Unable to initialize encoder.";

  AVFrame *frame = av_frame_alloc();   ///avcodec_alloc_frame();
	ASSERT_NE( frame, nullptr )	<< "Cannot create frame" ;

  frame->width = Width;
  frame->height = Height;
  frame->format = AV_PIX_FMT_RGB24;
	auto res = av_frame_get_buffer(frame, 0);
	ASSERT_GE(res, 0) << "Cannot allocate buffer";

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( size_t s = idx; s < idx+NumStreams; s++ ) {
      fillFrame( frame, Width, Height );

      writer->addFrame( frame, frameNum, s );
    }

    {
      // And data track
      AVPacket *pkt = av_packet_alloc();
      av_new_packet( pkt, 8 );

      pkt->stream_index = dataTrk;
      pkt->pts = int64_t(1000000 / FrameRate * frameNum);
      pkt->dts = pkt->pts;

      writer->addPacket( pkt );
    }

  }

  av_frame_free( &frame );

}
