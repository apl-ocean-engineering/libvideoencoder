
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

int main( int argc, char **argv )
{

  const int Width=320, Height=240;
  const int NumStreams = 2;
  const float FrameRate = 30.0;

  VideoWriter writer( "mov", AV_CODEC_ID_PRORES );
  std::array<VideoTrack,2> tracks = { VideoTrack( writer, Width, Height, FrameRate ),
                                      VideoTrack( writer, Width, Height, FrameRate ) };

  if( !writer.open( "/tmp/test.mov" ) ) {
    std::cerr << "Unable to initialize encoder." << std::endl;
    exit(-1);
  }

  std::cout << "Saving test movie to /tmp/test.mov" << std::endl;

  AVFrame *frame = tracks[0].makeFrame();

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( int s = 0; s < NumStreams; s++ ) {
      fillFrame( frame, Width, Height );

      tracks[s].addFrame( frame, frameNum );
    }
  }

  av_frame_free( &frame );


  return 0;
}
