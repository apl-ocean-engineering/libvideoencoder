
fips_begin_app( encode cmdline )
  fips_src( . )

  fips_deps( videoencoder )
  fips_libs( avutil avcodec swscale avformat )

fips_end_app()
