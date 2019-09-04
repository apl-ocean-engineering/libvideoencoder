/*
FFmpeg simple VideoWriter
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <ctime>

#include <iostream>

extern "C" {
  #include <libavutil/dict.h>
}

#include "libvideoencoder/VideoWriter.h"


namespace libvideoencoder {
  using namespace std;


  VideoWriter::VideoWriter( const std::string &container, const AVCodecID codec_id )
    : _codec(nullptr),
      _outFormatContext( avformat_alloc_context() )
  {
    av_log_set_level( AV_LOG_VERBOSE );

    _outFormatContext->oformat = av_guess_format(container.c_str(), NULL, NULL);
    assert(_outFormatContext->oformat != nullptr );  // Should be an exception?

    _codec = avcodec_find_encoder( codec_id );
    assert( _codec != nullptr );   // Should be an exception?

    describeCodec( _codec->id );
  }



  VideoWriter::VideoWriter( const std::string &container, const std::string &codec_name )
    : _codec(nullptr),
      _outFormatContext( avformat_alloc_context() )
  {
    av_log_set_level( AV_LOG_VERBOSE );

    _outFormatContext->oformat = av_guess_format(container.c_str(), NULL, NULL);
    assert(_outFormatContext->oformat != nullptr );  // Should be an exception?

    _codec = avcodec_find_encoder_by_name( codec_name.c_str() );
    assert( _codec != nullptr );   // Should be an exception?

    describeCodec( _codec->id );
  }


  VideoWriter::~VideoWriter()
  {
    close();

    if (_outFormatContext) av_free(_outFormatContext);
  }

  void VideoWriter::describeCodec( AVCodecID codec_id ) {
    // cout << "Using container format " << _outFormat->name << " (" << _outFormat->long_name << ")" << endl;
    //
    const AVCodecDescriptor *codecDesc = avcodec_descriptor_get(codec_id);
    if( codecDesc ) {
      std::cout << "Using codec " << codec_id << ": " << codecDesc->name << " (" << codecDesc->long_name << ")"  << std::endl;
    } else {
      std::cerr << "Could not retrieve codec description for " << codec_id << std::endl;
    }
  }



  bool VideoWriter::open( const std::string &inputFile )
  {
    assert( _outFormatContext != nullptr );

    av_dump_format(_outFormatContext, 0, inputFile.c_str(), 1);

    if (avio_open(&_outFormatContext->pb, inputFile.c_str(), AVIO_FLAG_WRITE) < 0)	{
      cerr << "Cannot open file" << endl;
      //Free();
      return false;
    }

    // AVDictionary *dict = nullptr;
    //av_dict_set_int( &dict, "write_tmcd", 1, 0 );

    // Set
    {
      std::time_t t = std::time(nullptr);
      char mbstr[100];
      std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S.00", std::localtime(&t));

      av_dict_set( &_outFormatContext->metadata, "timecode", mbstr, 0 );
    }

    auto result = avformat_write_header(_outFormatContext, nullptr );

    // av_dict_free( &dict );

    return (result == 0);
  }



  bool VideoWriter::close()
  {
    if( !_outFormatContext ) return false;
    if( !_outFormatContext->pb  )  return false;

    cerr << "Writing trailer" << endl;
    auto result = av_write_trailer(_outFormatContext);
    if( result != 0 ) {
      cerr << "Error writing AVIO trailer: " << std::hex << result;
    }

    //if (!(_outFormatContext->flags & AVFMT_NOFILE) ) {
    cerr << "Closing avio";
    avio_close(_outFormatContext->pb);
    _outFormatContext->pb = nullptr;
    //}

    return true;
  }



  bool VideoWriter::writePacket(AVPacket *packet )
  {
    assert( _outFormatContext );
    assert( packet->stream_index < _outFormatContext->nb_streams );

    {
      std::lock_guard<std::mutex> guard(_writeMutex);
      auto result = av_interleaved_write_frame(_outFormatContext, packet);
      av_packet_free( &packet );

      if( result < 0 ) {
        std::cerr << "Error writing interleaved packet" << std::endl;
        return false;
      }
    }

    return true;

  }

};
