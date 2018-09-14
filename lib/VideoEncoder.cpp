/*
FFmpeg simple Encoder
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <algorithm>

#include <iostream>

#include "libvideoencoder/VideoEncoder.h"

#include "libvideoencoder/utils.h"


#define MAX_AUDIO_PACKET_SIZE (128 * 1024)


namespace VideoEncoder {

  using namespace std;

  Encoder::Encoder( const int width, const int height )
  : _width(width), _height(height),
    _pOutFormat( nullptr ),
    _pFormatContext( nullptr )
  {
    //nSizeVideoEncodeBuffer = 0;

    av_log_set_level( AV_LOG_VERBOSE );
  }


  Encoder::~Encoder()
  {
    Finish();
  }


  bool Encoder::InitFile( const std::string &inputFile, const std::string &container, const AVCodecID codec)
  {
    // Initialize libavcodec
    avcodec_register_all();
    av_register_all();					// Initializes libavformat

    if (container == std::string("auto")) {
      _pOutFormat = av_guess_format(NULL, inputFile.c_str(), NULL);
    } else {
      _pOutFormat = av_guess_format(container.c_str(), NULL, NULL);
    }

    if (_pOutFormat == nullptr ) {
      cerr << "Unable to determine output format." << endl;
      return false;
    }

    cout << "Using container format " << _pOutFormat->name << " (" << _pOutFormat->long_name << ")" << endl;

    // allocate context
    _pFormatContext = avformat_alloc_context();
    if (!_pFormatContext) {
      cerr << "Unable to allocate avformat context" << endl;
      Free();
      return false;
    }

    _pFormatContext->oformat = _pOutFormat;
    memcpy(_pFormatContext->filename, inputFile.c_str(), std::min(inputFile.size(), sizeof(_pFormatContext->filename)));

    AVCodecID codec_id = codec;
    if( codec == AV_CODEC_ID_NONE ) codec_id = _pOutFormat->video_codec;;

    // Add video stream
    auto result = AddVideoStream( codec_id );
    if( !result ) {
      cerr << "Failed to add video stream";
      Free();
      return false;
    }


    {
      for( auto itr = _vosts.begin(); itr != _vosts.end(); ++itr ) {
        if( !(*itr)->open() ) {
          cerr << "Couldn't open stream..." << endl;
          return false;
        }
      }

      av_dump_format(_pFormatContext, 0, inputFile.c_str(), 1);


      // if (res && !(_pOutFormat->flags & AVFMT_NOFILE))
      // {

      if (avio_open(&_pFormatContext->pb, inputFile.c_str(), AVIO_FLAG_WRITE) < 0)	{
        cerr << "Cannot open file" << endl;
        Free();
        return false;
      }

      avformat_write_header(_pFormatContext, NULL);
    }

    return true;
  }



  bool Encoder::Finish()
  {
    bool res = true;

    if (_pFormatContext) {
      cerr << "Writing trailer" << endl;
      av_write_trailer(_pFormatContext);
      Free();
    }

    return res;
  }


  void Encoder::Free()
  {
    bool res = true;

    if (_pFormatContext)
    {
      // close video stream
      // Free the OutputStreams.

      // for(size_t i = 0; i < _pFormatContext->nb_streams; i++) {
      //   av_freep(&_pFormatContext->streams[i]->codec);
      //   av_freep(&_pFormatContext->streams[i]);
      // }

      if (!(_pFormatContext->flags & AVFMT_NOFILE) && _pFormatContext->pb) {
        avio_close(_pFormatContext->pb);
      }

      // Free the stream.
      av_free(_pFormatContext);
      _pFormatContext = NULL;
    }
  }



    int Encoder::AddVideoStream( AVCodecID codec_id )
    {

      shared_ptr<OutputStream> stream( new OutputStream( _pFormatContext ));

      if( !stream->init( codec_id, _width, _height ) ) {
        return -1;
      }

      _vosts.push_back( stream );

      //avcodec_parameters_to_context( st->codec, st->codecpar );

      // Some formats want stream headers to be separate.
      // if(pContext->oformat->flags & AVFMT_GLOBALHEADER)
      // {
      // 	pCodecCxt->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
      // }
      return _vosts.size()-1;
    }



//
//   void Encoder::CloseVideo(AVFormatContext *pContext, OutputStream *ost)
//   {
// //    avcodec_close(ost->enc);
//
//     // if (pCurrentPicture)
//     // {
//     //   if (pCurrentPicture->data)
//     //   {
//     //     av_free(pCurrentPicture->data[0]);
//     //     pCurrentPicture->data[0] = NULL;
//     //   }
//     //   av_free(pCurrentPicture);
//     //   pCurrentPicture = NULL;
//     // }
//     //
//     // if (pVideoEncodeBuffer)
//     // {
//     //   av_free(pVideoEncodeBuffer);
//     //   pVideoEncodeBuffer = NULL;
//     // }
//     // nSizeVideoEncodeBuffer = 0;
//   }




      bool Encoder::AddFrame(AVFrame* frame, unsigned int frameNum, unsigned int stream )
      {
        bool res = true;
        int nOutputSize = 0;
        AVCodecContext *pVideoCodec = NULL;

        auto ost = _vosts.at(stream);

        if (ost && frame && frame->data[0]) {
          //pVideoCodec = _pVideoStream->codec;
          ost->addFrame( frame, frameNum );
        }

        return res;
      }




    };
