#pragma once

extern "C"
{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/mathematics.h>
}

#include <vector>
#include <string>
#include <memory>

#include "libvideoencoder/OutputStream.h"

namespace libvideoencoder {

  class Encoder {
  public:


    Encoder( const int width, const int height, const float frameRate );

    virtual ~Encoder();

    // init output file
    bool InitFile(const std::string &inputFile, const std::string &container, const AVCodecID codec = AV_CODEC_ID_NONE, int numStreams = 1 );

    // Add video and audio data
    bool AddFrame(AVFrame* frame, unsigned int frameNum, unsigned int stream = 0 );

    // end of output
    bool Finish();

  private:

    // Add video stream
     int AddVideoStream(AVCodecID codec_id);
    // Open Video Stream

    //bool OpenVideo(AVFormatContext *oc, OutputStream *ost);

    // Close video stream
    //void CloseVideo(AVFormatContext *oc, OutputStream *ost);

    // Add audio stream
    // AVStream * AddAudioStream(AVFormatContext *pContext, AVCodecID codec_id);
    // // Open audio stream
    // bool OpenAudio(AVFormatContext *pContext, AVStream *pStream);
    // // close audio stream
    // void CloseAudio(AVFormatContext *pContext, AVStream *pStream);

    //== Add frames to movie ==

    // Add video frame to stream
    //bool AddVideoFrame(AVFrame *data, OutputStream *ost);

    // Add audio samples to stream
    //bool AddAudioSample(AVFormatContext *_pFormatContext, AVStream *pStream, const char* soundBuffer, int soundBufferSize);


    // Free all resourses.
    void Free();
    int _width, _height;
    float _frameRate;


    // output file name
    //std::string     _outputFilename;

    // output format and format context
    AVOutputFormat  *_pOutFormat;

    // format context
    AVFormatContext *_pFormatContext;

    std::vector< std::shared_ptr<OutputStream> > _vosts;

    // video stream context
    // AVStream * _pVideoStream;
    // // audio streams context
    //AVStream * pAudioStream;

    // // convert context context
    // struct SwsContext *_pImgConvertCtx;
    // // encode buffer and size
    // uint8_t * pVideoEncodeBuffer;
    // int nSizeVideoEncodeBuffer;

    // audio buffer and size
    // uint8_t * pAudioEncodeBuffer;
    // int nSizeAudioEncodeBuffer;


    //int numFrames;

    // count of sample
    // int audioInputSampleSize;
    // current picture
    AVFrame *pCurrentPicture;

    // audio buffer. Save rest samples in audioBuffer from previous audio frame.
    // char* audioBuffer;
    // int   nAudioBufferSize;
    // int   nAudioBufferSizeCurrent;

  };

};
