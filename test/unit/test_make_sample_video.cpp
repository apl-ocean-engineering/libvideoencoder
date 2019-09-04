
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <iostream>
using namespace std;

#include "libvideoencoder/VideoWriter.h"
using libvideoencoder::VideoWriter;

#include "libvideoencoder/OutputTrack.h"
using libvideoencoder::VideoTrack;
using libvideoencoder::DataTrack;



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
const float FrameRate = 29.97;
const string Extension("mov");



TEST(TestMakeSampleVideo, oneVideoTracks) {

  VideoWriter writer( "mov", AV_CODEC_ID_PRORES );
  VideoTrack track( writer, Width, Height, FrameRate );

  ASSERT_TRUE( writer.open( "/tmp/test_onevideo.mov" ) ) << "Unable to initialize encoder.";

  AVFrame *frame = track.allocateFrame();

  const int numFrames = 10;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {
    fillFrame( frame, Width, Height );
    writer.writePacket( track.encodeFrame( frame, frameNum ) );
  }

  av_frame_free( &frame );

}

TEST(TestMakeSampleVideo, twoVideoTracks) {

  VideoWriter writer( "mov", AV_CODEC_ID_PRORES );
  std::array<VideoTrack,2> tracks = { VideoTrack( writer, Width, Height, FrameRate ),
                                      VideoTrack( writer, Width, Height, FrameRate ) };

  ASSERT_TRUE( writer.open( "/tmp/test_twovideo.mov" ) ) << "Unable to initialize encoder.";

  AVFrame *frame = tracks[0].allocateFrame();

  const int numFrames = 10;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( size_t s = 0; s < NumStreams; ++s ) {
      fillFrame( frame, Width, Height );

      writer.writePacket( tracks[s].encodeFrame( frame, frameNum ) );
    }
  }

  av_frame_free( &frame );

}


TEST(TestMakeSampleVideo, twoVideoOneDataTrack) {

  VideoWriter writer( "mov", AV_CODEC_ID_PRORES );
  std::array<VideoTrack,2> videoTracks = { VideoTrack( writer, Width, Height, FrameRate ),
                                      VideoTrack( writer, Width, Height, FrameRate ) };

  DataTrack dataTrack( writer );

  ASSERT_TRUE( writer.open( "/tmp/test_twovideo_onedata." + Extension ) ) << "Unable to initialize encoder.";

  AVFrame *frame = videoTracks[0].allocateFrame();

  const int numFrames = 120;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( size_t s = 0; s < videoTracks.size(); s++ ) {
      fillFrame( frame, Width, Height );

      writer.writePacket( videoTracks[s].encodeFrame( frame, frameNum ) );
    }

    {
      size_t len = rand() % 32;
      char *data = dataTrack.allocate(len);

      writer.writePacket( dataTrack.encodeData( data, len ) );
    }

  }

  av_frame_free( &frame );

}
