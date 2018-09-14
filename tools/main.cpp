
#include <string>
#include <iostream>
using namespace std;

#include "libvideoencoder/VideoEncoder.h"
using VideoEncoder::Encoder;

void setPixel(AVFrame *pFrame, short x, short y, short red, short green, short blue ) {

    unsigned int off = (y * pFrame->linesize[0]) + (3*x);

    pFrame->data[0][off] = (uint8_t)red;
    pFrame->data[0][off + 1] = (uint8_t)green;
    pFrame->data[0][off + 2] = (uint8_t)blue;

}

int main( int argc, char **argv ) {

  const int Width=320, Height=240;
  const int NumStreams = 2;
  Encoder encoder(Width,Height);

  auto result = encoder.InitFile("/tmp/test.mov", "auto", AV_CODEC_ID_PRORES, NumStreams );
  if( !result ) {
    std::cerr << "Unable to initialize encoder." << std::endl;
    exit(-1);
  }


  AVFrame *frame = av_frame_alloc();   ///avcodec_alloc_frame();
	if ( !frame )	{
		cerr << "Cannot create frame" << endl;
		exit(-1);
	}

  frame->width = Width;
  frame->height = Height;
  frame->format = AV_PIX_FMT_RGB24;

	auto res = av_frame_get_buffer(frame, 0);

	if (res < 0) {
		av_frame_free( &frame);
		printf("Cannot allocate buffer\n");
		exit(-1);
	}

  const int numFrames = 240;

  for( int frameNum = 0; frameNum < numFrames; ++frameNum ) {

    for( int s = 0; s < NumStreams; s++ ) {
    // Fill buffer with random noise
      for( unsigned int x = 0; x < Width; ++x  ) {
        for( unsigned int y = 0; y < Height; ++y ) {
          setPixel( frame, x, y, rand() % 256, rand() % 256, rand() % 256  );
        }
      }

    encoder.AddFrame( frame, frameNum, s );
  }
  }

  av_frame_free( &frame );


  return 0;
}
