
#include <gtest/gtest.h>

#include <thread>
#include <array>
#include <string>
#include <iostream>
using namespace std;

#include "libvideoencoder/VideoWriter.h"
using libvideoencoder::VideoWriter;

#include "libvideoencoder/OutputTrack.h"
using libvideoencoder::VideoTrack;



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

void addFrame( const shared_ptr<VideoTrack> &track, int frameNum, int stream ) {
  AVFrame *frame = track->allocateFrame();

  fillFrame( frame, Width, Height );

  track->writeFrame( frame, frameNum );

  av_frame_free( &frame );

}


// TEST(TestMakeSampleVideo, twoVideoTracksThreaded) {
//
//   shared_ptr<VideoWriter> writer( new VideoWriter( "mov", AV_CODEC_ID_PRORES ) );
//   shared_ptr<VideoTrack> track( *writer, Width, Height, FrameRate, NumStreams );
//
//   ASSERT_TRUE( writer->open( "/tmp/test_twovideo_threaded.mov" ) ) << "Unable to initialize encoder.";
//
//   const int numFrames = 240;
//
//   std::array< std::unique_ptr<std::thread>, NumStreams > threads;
//
//   for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {
//
//     for( size_t s = 0; s < NumStreams; s++ ) {
//       threads[s].reset( new std::thread( &addFrame, track, frameNum, s ) );
//     }
//
//     for( size_t s = idx; s < idx+NumStreams; s++ ) {
//       threads[s]->join();
//       threads[s].release();
//     }
//
//   }
//
//
//
// }
